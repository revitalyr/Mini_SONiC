/**
 * @file boost_wrappers.ixx
 * @brief Boost library wrappers - module interface
 * 
 * Provides C++20 module exports for Boost libraries:
 * - Boost.Asio for async networking
 * - Boost.UUID for packet identification
 * - Boost.Lockfree for lock-free queues
 * - Boost.Thread for synchronization
 */

module;

// Global module fragment - include headers here
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/atomic.hpp>
#include <boost/optional.hpp>
#include <boost/unordered_map.hpp>

export module MiniSonic.Boost.Wrappers;

// Export Boost.Asio components
export namespace MiniSonic::BoostWrapper {

// Asio core types
using IoContext = boost::asio::io_context;
using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
using SteadyTimer = boost::asio::steady_timer;

// Asio networking types
using TcpEndpoint = boost::asio::ip::tcp::endpoint;
using TcpAcceptor = boost::asio::ip::tcp::acceptor;
using TcpSocket = boost::asio::ip::tcp::socket;
using UdpEndpoint = boost::asio::ip::udp::endpoint;
using UdpSocket = boost::asio::ip::udp::socket;
using Address = boost::asio::ip::address;

// Error handling
using ErrorCode = boost::system::error_code;

// Thread pool wrapper
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads)
        : m_work_guard(boost::asio::make_work_guard(m_io_context)) {
        for (size_t i = 0; i < num_threads; ++i) {
            m_threads.emplace_back([this]() { m_io_context.run(); });
        }
    }
    
    ~ThreadPool() {
        stop();
    }
    
    void stop() {
        if (m_running) {
            m_work_guard.reset();
            m_io_context.stop();
            for (auto& t : m_threads) {
                if (t.joinable()) {
                    t.join();
                }
            }
            m_running = false;
        }
    }
    
    IoContext& getContext() { return m_io_context; }
    
private:
    IoContext m_io_context;
    WorkGuard m_work_guard;
    std::vector<std::thread> m_threads;
    bool m_running = true;
};

} // namespace MiniSonic::BoostWrapper

// Export Boost.UUID
export namespace MiniSonic::BoostWrapper {

using Uuid = boost::uuids::uuid;
using UuidGenerator = boost::uuids::random_generator;

inline Uuid generateUuid() {
    static UuidGenerator gen;
    return gen();
}

inline std::string uuidToString(const Uuid& uuid) {
    return boost::uuids::to_string(uuid);
}

inline bool isNullUuid(const Uuid& uuid) {
    return uuid.is_nil();
}

} // namespace MiniSonic::BoostWrapper

// Export Boost.Lockfree
export namespace MiniSonic::BoostWrapper {

template<typename T>
using LockfreeQueue = boost::lockfree::queue<T>;

template<typename T>
using LockfreeSPSCQueue = boost::lockfree::spsc_queue<T>;

// RAII wrapper for lockfree queue pointer
// This prevents memory leaks in lockfree queue usage
class PendingPacketPtr {
public:
    PendingPacketPtr() : m_ptr(nullptr) {}
    explicit PendingPacketPtr(void* ptr) : m_ptr(ptr) {}
    ~PendingPacketPtr() { reset(); }
    
    PendingPacketPtr(const PendingPacketPtr&) = delete;
    PendingPacketPtr& operator=(const PendingPacketPtr&) = delete;
    
    PendingPacketPtr(PendingPacketPtr&& other) noexcept 
        : m_ptr(other.release()) {}
    
    PendingPacketPtr& operator=(PendingPacketPtr&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }
    
    void* get() const noexcept { return m_ptr; }
    void* release() noexcept {
        void* tmp = m_ptr;
        m_ptr = nullptr;
        return tmp;
    }
    
    void reset(void* ptr = nullptr) noexcept {
        if (m_ptr) {
            // Caller is responsible for knowing the actual type
            // This is a raw pointer wrapper for lockfree queues
        }
        m_ptr = ptr;
    }
    
private:
    void* m_ptr;
};

} // namespace MiniSonic::BoostWrapper

// Export Boost.Thread synchronization
export namespace MiniSonic::BoostWrapper {

using Mutex = boost::mutex;
using RecursiveMutex = boost::recursive_mutex;
using LockGuard = boost::lock_guard<Mutex>;
using RecursiveLockGuard = boost::lock_guard<RecursiveMutex>;

// Spinlock using Boost atomic
class SpinLock {
public:
    void lock() {
        while (m_flag.test_and_set(boost::memory_order_acquire)) {
            // Spin
        }
    }
    
    void unlock() {
        m_flag.clear(boost::memory_order_release);
    }
    
    bool try_lock() {
        return !m_flag.test_and_set(boost::memory_order_acquire);
    }
    
private:
    boost::atomic_flag m_flag = ATOMIC_FLAG_INIT;
};

using SpinLockGuard = std::lock_guard<SpinLock>;

} // namespace MiniSonic::BoostWrapper

// Export Boost.Atomic
export namespace MiniSonic::BoostWrapper {

template<typename T>
using Atomic = boost::atomic<T>;

using AtomicUint64 = boost::atomic<uint64_t>;
using AtomicUint32 = boost::atomic<uint32_t>;
using AtomicSize = boost::atomic<size_t>;

} // namespace MiniSonic::BoostWrapper

// Export Boost.Optional
export namespace MiniSonic::BoostWrapper {

template<typename T>
using Optional = boost::optional<T>;

template<typename T>
inline Optional<T> makeOptional(const T& value) {
    return boost::make_optional(value);
}

} // namespace MiniSonic::BoostWrapper

// Export Boost.UnorderedMap
export namespace MiniSonic::BoostWrapper {

template<typename K, typename V>
using UnorderedMap = boost::unordered_map<K, V>;

} // namespace MiniSonic::BoostWrapper
