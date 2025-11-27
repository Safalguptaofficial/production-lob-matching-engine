#include "lob/market_data_publisher.hpp"
#include <chrono>
#include <thread>

namespace lob {

MarketDataPublisher::MarketDataPublisher(size_t queue_capacity) : queue_(queue_capacity) {}

MarketDataPublisher::~MarketDataPublisher() {
    stop();
}

void MarketDataPublisher::start(EventCallback callback) {
    if (running_.load()) {
        return;  // Already running
    }
    
    running_.store(true);
    publisher_thread_ = std::make_unique<std::thread>([this, callback]() {
        publisher_loop(callback);
    });
}

void MarketDataPublisher::stop() {
    if (!running_.load()) {
        return;  // Not running
    }
    
    running_.store(false);
    
    if (publisher_thread_ && publisher_thread_->joinable()) {
        publisher_thread_->join();
    }
}

bool MarketDataPublisher::publish_trade(const TradeEvent& event) {
    if (!running_.load()) {
        ++events_dropped_;
        return false;
    }
    
    if (!queue_.try_enqueue(event)) {
        ++events_dropped_;
        return false;
    }
    
    ++events_published_;
    return true;
}

void MarketDataPublisher::publisher_loop(EventCallback callback) {
    while (running_.load()) {
        auto event = queue_.try_dequeue();
        
        if (event) {
            callback(*event);
        } else {
            // Queue empty, sleep briefly to avoid spinning
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    
    // Drain remaining events
    while (auto event = queue_.try_dequeue()) {
        callback(*event);
    }
}

} // namespace lob

