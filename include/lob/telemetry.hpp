#pragma once

#include "types.hpp"
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace lob {

// Per-symbol statistics
struct SymbolStats {
    uint64_t active_orders = 0;
    uint64_t bid_levels = 0;
    uint64_t ask_levels = 0;
    uint64_t trade_volume = 0;
    uint64_t trade_count = 0;
    uint64_t max_bid_depth = 0;
    uint64_t max_ask_depth = 0;
    Price best_bid = INVALID_PRICE;
    Price best_ask = INVALID_PRICE;
    
    nlohmann::json to_json() const;
};

// Engine-wide telemetry
class Telemetry {
public:
    Telemetry() = default;
    
    // Order metrics
    void record_order_processed();
    void record_order_accepted();
    void record_order_rejected();
    void record_order_cancelled();
    
    // Trade metrics
    void record_trade(const std::string& symbol, Quantity quantity);
    
    // Latency metrics (nanoseconds)
    void record_latency(uint64_t latency_ns);
    
    // Book metrics
    void update_symbol_stats(const std::string& symbol, const SymbolStats& stats);
    
    // Getters
    uint64_t orders_processed() const { return orders_processed_; }
    uint64_t orders_accepted() const { return orders_accepted_; }
    uint64_t orders_rejected() const { return orders_rejected_; }
    uint64_t orders_cancelled() const { return orders_cancelled_; }
    uint64_t total_trades() const { return total_trades_; }
    
    uint64_t avg_latency_ns() const {
        return latency_count_ > 0 ? total_latency_ns_ / latency_count_ : 0;
    }
    
    uint64_t max_latency_ns() const { return max_latency_ns_; }
    uint64_t min_latency_ns() const { return min_latency_ns_; }
    
    const SymbolStats* get_symbol_stats(const std::string& symbol) const;
    
    // Export all metrics as JSON
    nlohmann::json to_json() const;
    
    // Reset all metrics
    void reset();
    
    // Estimate memory footprint (approximate)
    size_t estimate_memory_bytes() const;
    
private:
    uint64_t orders_processed_ = 0;
    uint64_t orders_accepted_ = 0;
    uint64_t orders_rejected_ = 0;
    uint64_t orders_cancelled_ = 0;
    uint64_t total_trades_ = 0;
    
    uint64_t total_latency_ns_ = 0;
    uint64_t latency_count_ = 0;
    uint64_t max_latency_ns_ = 0;
    uint64_t min_latency_ns_ = UINT64_MAX;
    
    std::unordered_map<std::string, SymbolStats> symbol_stats_;
};

} // namespace lob

