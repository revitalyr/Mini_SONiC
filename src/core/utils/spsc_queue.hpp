#pragma once

#include "common/types.hpp"
#include <atomic>
#include <vector>
#include <cstddef>

namespace MiniSonic::Utils {

/**
 * @brief Lock-free Single Producer Single Consumer (SPSC) Queue
 * 
 * High-performance queue for packet buffering between threads.
 * Only one thread can push (producer) and one thread can pop (consumer).
 * Uses atomic operations for thread safety without locks.
 * 
 * @tparam T Type of elements to store in the queue
 */
template<typename T>
class SPSCQueue {
public:
    /**
     * @brief Construct SPSC queue with specified capacity
     * @param size Maximum number of elements the queue can hold
     */
    explicit SPSCQueue(Types::Count size)
        : m_buffer(size), m_capacity(size) {}

    /**
     * @brief Push element to queue (producer only)
     * @param item Element to push
     * @return true if element was pushed, false if queue is full
     */
    bool push(const T& item) noexcept {
        const auto current_head = m_head.load(std::memory_order_relaxed);
        const auto next_head = (current_head + 1) % m_capacity;
        
        if (next_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        m_buffer[current_head] = item;
        m_head.store(next_head, std::memory_order_release);
        return true;
    }

    /**
     * @brief Push element to queue using move semantics (producer only)
     * @param item Element to push (will be moved)
     * @return true if element was pushed, false if queue is full
     */
    bool push(T&& item) noexcept {
        const auto current_head = m_head.load(std::memory_order_relaxed);
        const auto next_head = (current_head + 1) % m_capacity;
        
        if (next_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        m_buffer[current_head] = std::move(item);
        m_head.store(next_head, std::memory_order_release);
        return true;
    }

    /**
     * @brief Pop element from queue (consumer only)
     * @param item Reference to store popped element
     * @return true if element was popped, false if queue is empty
     */
    bool pop(T& item) noexcept {
        const auto current_tail = m_tail.load(std::memory_order_relaxed);
        
        if (current_tail == m_head.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }
        
        item = std::move(m_buffer[current_tail]);
        m_tail.store((current_tail + 1) % m_capacity, std::memory_order_release);
        return true;
    }

    /**
     * @brief Check if queue is empty
     * @return true if queue is empty
     */
    [[nodiscard]] bool empty() const noexcept {
        return m_head.load(std::memory_order_acquire) == 
               m_tail.load(std::memory_order_acquire);
    }

    /**
     * @brief Check if queue is full
     * @return true if queue is full
     */
    [[nodiscard]] bool full() const noexcept {
        const auto next_head = (m_head.load(std::memory_order_acquire) + 1) % m_capacity;
        return next_head == m_tail.load(std::memory_order_acquire);
    }

    /**
     * @brief Get current size of queue
     * @return Approximate number of elements in queue
     */
    [[nodiscard]] Types::Count size() const noexcept {
        const auto head = m_head.load(std::memory_order_acquire);
        const auto tail = m_tail.load(std::memory_order_acquire);
        
        if (head >= tail) {
            return head - tail;
        } else {
            return m_capacity - tail + head;
        }
    }

    /**
     * @brief Get queue capacity
     * @return Maximum number of elements queue can hold
     */
    [[nodiscard]] Types::Count capacity() const noexcept {
        return m_capacity;
    }

private:
    std::vector<T> m_buffer;
    const Types::Count m_capacity;
    alignas(64) std::atomic<Types::Count> m_head{0};  // Producer index
    alignas(64) std::atomic<Types::Count> m_tail{0};  // Consumer index
};

} // namespace MiniSonic::Utils
