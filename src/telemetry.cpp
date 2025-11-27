#include "lob/telemetry.hpp"

namespace lob {

// ============================================================================
// SymbolStats Implementation
// ============================================================================

nlohmann::json SymbolStats::to_json() const {
    nlohmann::json j;
    j["active_orders"] = active_orders;
    j["bid_levels"] = bid_levels;
    j["ask_levels"] = ask_levels;
    j["trade_volume"] = trade_volume;
    j["trade_count"] = trade_count;
    j["max_bid_depth"] = max_bid_depth;
    j["max_ask_depth"] = max_ask_depth;
    j["best_bid"] = best_bid;
    j["best_ask"] = best_ask;
    return j;
}

// ============================================================================
// Telemetry Implementation
// ============================================================================

void Telemetry::record_order_processed() {
    ++orders_processed_;
}

void Telemetry::record_order_accepted() {
    ++orders_accepted_;
}

void Telemetry::record_order_rejected() {
    ++orders_rejected_;
}

void Telemetry::record_order_cancelled() {
    ++orders_cancelled_;
}

void Telemetry::record_trade(const std::string& symbol, Quantity quantity) {
    ++total_trades_;
    
    auto& stats = symbol_stats_[symbol];
    stats.trade_count++;
    stats.trade_volume += quantity;
}

void Telemetry::record_latency(uint64_t latency_ns) {
    total_latency_ns_ += latency_ns;
    ++latency_count_;
    
    if (latency_ns > max_latency_ns_) {
        max_latency_ns_ = latency_ns;
    }
    
    if (latency_ns < min_latency_ns_) {
        min_latency_ns_ = latency_ns;
    }
}

void Telemetry::update_symbol_stats(const std::string& symbol, const SymbolStats& stats) {
    symbol_stats_[symbol] = stats;
}

const SymbolStats* Telemetry::get_symbol_stats(const std::string& symbol) const {
    auto it = symbol_stats_.find(symbol);
    return it != symbol_stats_.end() ? &it->second : nullptr;
}

nlohmann::json Telemetry::to_json() const {
    nlohmann::json j;
    
    // Engine-wide metrics
    j["orders_processed"] = orders_processed_;
    j["orders_accepted"] = orders_accepted_;
    j["orders_rejected"] = orders_rejected_;
    j["orders_cancelled"] = orders_cancelled_;
    j["total_trades"] = total_trades_;
    
    // Latency metrics
    j["avg_latency_ns"] = avg_latency_ns();
    j["max_latency_ns"] = max_latency_ns_;
    j["min_latency_ns"] = (min_latency_ns_ == UINT64_MAX) ? 0 : min_latency_ns_;
    
    // Per-symbol metrics
    j["symbols"] = nlohmann::json::object();
    for (const auto& [symbol, stats] : symbol_stats_) {
        j["symbols"][symbol] = stats.to_json();
    }
    
    // Memory estimate
    j["memory_bytes_estimate"] = estimate_memory_bytes();
    
    return j;
}

void Telemetry::reset() {
    orders_processed_ = 0;
    orders_accepted_ = 0;
    orders_rejected_ = 0;
    orders_cancelled_ = 0;
    total_trades_ = 0;
    total_latency_ns_ = 0;
    latency_count_ = 0;
    max_latency_ns_ = 0;
    min_latency_ns_ = UINT64_MAX;
    symbol_stats_.clear();
}

size_t Telemetry::estimate_memory_bytes() const {
    // Rough estimate
    size_t base = sizeof(Telemetry);
    size_t map_overhead = symbol_stats_.size() * (sizeof(std::string) + sizeof(SymbolStats) + 64);
    return base + map_overhead;
}

} // namespace lob

