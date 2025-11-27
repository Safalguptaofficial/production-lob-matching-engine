#pragma once

#include <atomic>
#include <vector>
#include <optional>
#include <cstddef>

namespace lob {

// Single-producer single-consumer lock-free ring buffer
// Cache-line aligned to prevent false sharing
template<typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t capacity)
        : capacity_(round_up_power_of_2(capacity)),
          mask_(capacity_ - 1),
          buffer_(capacity_),
          head_(0),
          tail_(0) {
    }
    
    // Producer: try to enqueue an item
    bool try_enqueue(const T& item) {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = (current_tail + 1) & mask_;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    // Producer: try to enqueue with move semantics
    bool try_enqueue(T&& item) {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = (current_tail + 1) & mask_;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        buffer_[current_tail] = std::move(item);
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    // Consumer: try to dequeue an item
    std::optional<T> try_dequeue() {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt; // Queue is empty
        }
        
        T item = std::move(buffer_[current_head]);
        head_.store((current_head + 1) & mask_, std::memory_order_release);
        return item;
    }
    
    // Check if queue is empty
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    // Get approximate size (may be inaccurate due to concurrency)
    size_t size() const {
        const size_t h = head_.load(std::memory_order_acquire);
        const size_t t = tail_.load(std::memory_order_acquire);
        return (t >= h) ? (t - h) : (capacity_ - h + t);
    }
    
    size_t capacity() const { return capacity_ - 1; } // -1 because one slot is reserved
    
private:
    const size_t capacity_;
    const size_t mask_;
    std::vector<T> buffer_;
    
    // Cache-line aligned atomics (64 bytes is typical cache line size)
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
    
    // Round up to nearest power of 2
    static size_t round_up_power_of_2(size_t n) {
        if (n <= 1) return 2;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }
};

} // namespace lob

