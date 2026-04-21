module;

#include <string> // For std::string
#include <functional> // For std::function
#include <memory> // For std::unique_ptr, std::shared_ptr
#include <vector> // For std::vector
#include <map> // For std::map
#include <atomic> // For std::atomic
#include <mutex> // For std::mutex, std::lock_guard
#include <variant> // For std::variant
#include <cstddef> // For std::byte
#include <iostream> // For std::cout, std::cerr
#include <utility> // For std::move

module MiniSonic.Protocol;

namespace MiniSonic::Protocol {

using std::string;
using std::function;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::map;
using std::atomic;
using std::mutex;
using std::variant;
using std::byte;
using std::lock_guard;

// =============================================================================
// PROTOCOL STACK IMPLEMENTATION
// =============================================================================

ProtocolStack::~ProtocolStack() {
    stopAll();
}

void ProtocolStack::registerHandler(const string& name, unique_ptr<IProtocolHandler> handler) {
    lock_guard<mutex> lock(m_mutex);
    m_handlers[name] = std::move(handler);
    std::cout << "[ProtocolStack] Registered handler: " << name << "\n";
}

void ProtocolStack::unregisterHandler(const string& name) {
    lock_guard<mutex> lock(m_mutex);
    auto it = m_handlers.find(name);
    if (it != m_handlers.end()) {
        it->second->stop();
        m_handlers.erase(it);
        std::cout << "[ProtocolStack] Unregistered handler: " << name << "\n";
    }
}

IProtocolHandler* ProtocolStack::getHandler(const string& name) const {
    lock_guard<mutex> lock(m_mutex);
    auto it = m_handlers.find(name);
    return it != m_handlers.end() ? it->second.get() : nullptr;
}

void ProtocolStack::startAll() {
    lock_guard<mutex> lock(m_mutex);
    for (auto& [name, handler] : m_handlers) {
        try {
            handler->start();
            std::cout << "[ProtocolStack] Started handler: " << name << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[ProtocolStack] Failed to start " << name << ": " << e.what() << "\n";
        }
    }
}

void ProtocolStack::stopAll() {
    lock_guard<mutex> lock(m_mutex);
    for (auto& [name, handler] : m_handlers) {
        try {
            handler->stop();
            std::cout << "[ProtocolStack] Stopped handler: " << name << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[ProtocolStack] Failed to stop " << name << ": " << e.what() << "\n";
        }
    }
}

void ProtocolStack::send(const string& protocol_name, const Message& msg) {
    lock_guard<mutex> lock(m_mutex);
    auto it = m_handlers.find(protocol_name);
    if (it != m_handlers.end()) {
        it->second->send(msg);
    } else {
        std::cerr << "[ProtocolStack] Handler not found: " << protocol_name << "\n";
    }
}

void ProtocolStack::broadcast(const Message& msg) {
    lock_guard<mutex> lock(m_mutex);
    for (auto& [name, handler] : m_handlers) {
        handler->broadcast(msg);
    }
}

void ProtocolStack::setGlobalMessageHandler(function<void(const string&, const Message&)> handler) {
    lock_guard<mutex> lock(m_mutex);
    m_global_handler = std::move(handler);
    
    // Propagate to all handlers
    for (auto& [name, h] : m_handlers) {
        h->setMessageHandler([this, name](const Message& msg) {
            if (m_global_handler) {
                m_global_handler(name, msg);
            }
        });
    }
}

string ProtocolStack::getAllStats() const {
    lock_guard<mutex> lock(m_mutex);
    string stats = "=== Protocol Stack Statistics ===\n";
    for (const auto& [name, handler] : m_handlers) {
        stats += "\n[" + name + "]\n";
        stats += handler->getStats();
    }
    return stats;
}

} // namespace MiniSonic::Protocol
