#pragma once

#include "events.hpp"
#include "lockfree_queue.hpp"
#include <thread>
#include <atomic>
#include <memory>
#include <functional>

namespace lob {

// Market data publisher using lock-free queue
// Mimics exchange market data feeds (CME MDP3, NASDAQ ITCH style)
class MarketDataPublisher {
public:
    using EventCallback = std::function<void(const TradeEvent&)>;
    
    explicit MarketDataPublisher(size_t queue_capacity = 65536);
    ~MarketDataPublisher();
    
    // Start publisher thread
    void start(EventCallback callback);
    
    // Stop publisher thread
    void stop();
    
    // Publish event (called by matching engine)
    bool publish_trade(const TradeEvent& event);
    
    // Statistics
    uint64_t events_published() const { return events_published_.load(); }
    uint64_t events_dropped() const { return events_dropped_.load(); }
    
    bool is_running() const { return running_.load(); }
    
private:
    LockFreeQueue<TradeEvent> queue_;
    std::unique_ptr<std::thread> publisher_thread_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> events_published_{0};
    std::atomic<uint64_t> events_dropped_{0};
    
    void publisher_loop(EventCallback callback);
};

} // namespace lob

