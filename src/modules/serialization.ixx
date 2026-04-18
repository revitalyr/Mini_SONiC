module;

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>

export module MiniSonic.Serialization;

import MiniSonic.Utils;

using std::string;
using std::vector;
using std::unique_ptr;

export namespace MiniSonic::Serialization {

// =============================================================================
// FLATBUFFERS SERIALIZATION ABSTRACTION
// =============================================================================

/**
 * @brief Serialization result with buffer and size
 */
struct SerializedData {
    vector<uint8_t> buffer;
    size_t size{0};
    
    [[nodiscard]] bool isValid() const noexcept {
        return !buffer.empty() && size > 0;
    }
    
    [[nodiscard]] const uint8_t* data() const noexcept {
        return buffer.data();
    }
};

/**
 * @brief Abstract serializer interface
 * 
 * Provides zero-copy serialization abstraction using FlatBuffers.
 */
class ISerializer {
public:
    virtual ~ISerializer() = default;
    
    /**
     * @brief Serialize data to buffer
     */
    [[nodiscard]] virtual SerializedData serialize(const void* data, size_t size) = 0;
    
    /**
     * @brief Deserialize data from buffer (zero-copy access)
     */
    [[nodiscard]] virtual const void* deserialize(const uint8_t* buffer, size_t size) = 0;
    
    /**
     * @brief Get serialized size without allocating
     */
    [[nodiscard]] virtual size_t getSerializedSize(const void* data, size_t size) const = 0;
};

/**
 * @brief FlatBuffers-based serializer
 * 
 * Provides zero-copy serialization using FlatBuffers library.
 * This is a wrapper that can be extended with actual FlatBuffers schema.
 */
class FlatBufferSerializer : public ISerializer {
public:
    FlatBufferSerializer() = default;
    ~FlatBufferSerializer() override = default;
    
    /**
     * @brief Serialize data to FlatBuffer format
     */
    [[nodiscard]] SerializedData serialize(const void* data, size_t size) override {
        SerializedData result;
        result.buffer.resize(size + sizeof(size_t));
        
        // Store size prefix
        uint32_t size_prefix = static_cast<uint32_t>(size);
        std::memcpy(result.buffer.data(), &size_prefix, sizeof(size_prefix));
        
        // Copy data (in real implementation, this would use FlatBuffers builder)
        std::memcpy(result.buffer.data() + sizeof(size_prefix), data, size);
        result.size = result.buffer.size();
        
        return result;
    }
    
    /**
     * @brief Deserialize from FlatBuffer (zero-copy access)
     */
    [[nodiscard]] const void* deserialize(const uint8_t* buffer, size_t size) override {
        if (size < sizeof(uint32_t)) {
            return nullptr;
        }
        
        // Read size prefix
        uint32_t data_size;
        std::memcpy(&data_size, buffer, sizeof(data_size));
        
        if (size < sizeof(data_size) + data_size) {
            return nullptr;
        }
        
        // Return pointer to data (zero-copy)
        return buffer + sizeof(data_size);
    }
    
    /**
     * @brief Get serialized size
     */
    [[nodiscard]] size_t getSerializedSize(const void* data, size_t size) const override {
        return size + sizeof(uint32_t);
    }
};

// =============================================================================
// PROTOCOL-SPECIFIC SERIALIZERS
// =============================================================================

/**
 * @brief Network packet serializer
 */
class PacketSerializer {
public:
    /**
     * @brief Serialize MAC address to buffer
     */
    static vector<uint8_t> serializeMac(const Types::MacAddress& mac) {
        vector<uint8_t> buffer(6);
        // MAC address is typically 6 bytes
        // In real implementation, extract from MacAddress type
        for (size_t i = 0; i < 6 && i < mac.size(); ++i) {
            buffer[i] = static_cast<uint8_t>(mac[i]);
        }
        return buffer;
    }
    
    /**
     * @brief Deserialize MAC address from buffer
     */
    static Types::MacAddress deserializeMac(const uint8_t* buffer, size_t size) {
        if (size < 6) {
            return Types::MacAddress{};
        }
        
        Types::MacAddress mac;
        for (size_t i = 0; i < 6; ++i) {
            mac[i] = buffer[i];
        }
        return mac;
    }
    
    /**
     * @brief Serialize IP address to buffer
     */
    static vector<uint8_t> serializeIp(const Types::IpAddress& ip) {
        // IPv4 is 4 bytes, IPv6 is 16 bytes
        vector<uint8_t> buffer;
        buffer.reserve(ip.size());
        
        for (size_t i = 0; i < ip.size(); ++i) {
            buffer.push_back(static_cast<uint8_t>(ip[i]));
        }
        
        return buffer;
    }
    
    /**
     * @brief Deserialize IP address from buffer
     */
    static Types::IpAddress deserializeIp(const uint8_t* buffer, size_t size) {
        Types::IpAddress ip;
        
        if (size == 4) {
            // IPv4
            for (size_t i = 0; i < 4; ++i) {
                ip[i] = buffer[i];
            }
        } else if (size == 16) {
            // IPv6
            for (size_t i = 0; i < 16; ++i) {
                ip[i] = buffer[i];
            }
        }
        
        return ip;
    }
};

/**
 * @brief Protocol message serializer
 */
class MessageSerializer {
public:
    /**
     * @brief Serialize protocol message to buffer
     */
    static vector<uint8_t> serializeMessage(uint32_t type, const vector<uint8_t>& payload) {
        // Header: type (4 bytes) + length (4 bytes)
        vector<uint8_t> buffer;
        buffer.reserve(sizeof(uint32_t) * 2 + payload.size());
        
        // Write type
        uint32_t type_be = htonl(type);
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&type_be), 
                      reinterpret_cast<uint8_t*>(&type_be) + sizeof(uint32_t));
        
        // Write length
        uint32_t length_be = htonl(static_cast<uint32_t>(payload.size()));
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&length_be),
                      reinterpret_cast<uint8_t*>(&length_be) + sizeof(uint32_t));
        
        // Write payload
        buffer.insert(buffer.end(), payload.begin(), payload.end());
        
        return buffer;
    }
    
    /**
     * @brief Deserialize protocol message from buffer
     */
    static bool deserializeMessage(const uint8_t* buffer, size_t size,
                                     uint32_t& out_type, vector<uint8_t>& out_payload) {
        if (size < sizeof(uint32_t) * 2) {
            return false;
        }
        
        // Read type
        uint32_t type_be;
        std::memcpy(&type_be, buffer, sizeof(uint32_t));
        out_type = ntohl(type_be);
        
        // Read length
        uint32_t length_be;
        std::memcpy(&length_be, buffer + sizeof(uint32_t), sizeof(uint32_t));
        uint32_t payload_size = ntohl(length_be);
        
        // Validate payload size
        if (size < sizeof(uint32_t) * 2 + payload_size) {
            return false;
        }
        
        // Extract payload
        out_payload.assign(buffer + sizeof(uint32_t) * 2,
                          buffer + sizeof(uint32_t) * 2 + payload_size);
        
        return true;
    }
};

// =============================================================================
// SERIALIZATION FACTORY
// =============================================================================

/**
 * @brief Factory for creating serializers
 */
class SerializerFactory {
public:
    static unique_ptr<ISerializer> createFlatBufferSerializer() {
        return std::make_unique<FlatBufferSerializer>();
    }
    
    static PacketSerializer createPacketSerializer() {
        return PacketSerializer{};
    }
    
    static MessageSerializer createMessageSerializer() {
        return MessageSerializer{};
    }
};

} // export namespace MiniSonic::Serialization
