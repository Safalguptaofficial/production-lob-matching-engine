#pragma once

#include "types.hpp"
#include "order_book.hpp"
#include "messages.hpp"
#include "listener.hpp"
#include "event_log.hpp"
#include "telemetry.hpp"
#include "trade_tape.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <chrono>

namespace lob {

// Symbol configuration
struct SymbolConfig {
    std::string symbol;
    Price tick_size = 1;        // Minimum price increment
    Quantity lot_size = 1;      // Minimum quantity increment
    Quantity min_quantity = 1;  // Minimum order quantity
    STPPolicy stp_policy = STPPolicy::CANCEL_INCOMING;
    
    bool is_valid() const {
        return !symbol.empty() && tick_size > 0 && lot_size > 0 && min_quantity > 0;
    }
};

// Multi-symbol matching engine
class MatchingEngine {
public:
    MatchingEngine();
    explicit MatchingEngine(bool deterministic);
    
    // Symbol management
    bool add_symbol(const SymbolConfig& config);
    bool has_symbol(const std::string& symbol) const;
    
    // Order operations
    OrderResponse handle(const NewOrderRequest& request);
    OrderResponse handle(const CancelRequest& request);
    OrderResponse handle(const ReplaceRequest& request);
    
    // Market data queries
    TopOfBook get_top_of_book(const std::string& symbol, Timestamp timestamp = 0) const;
    DepthSnapshot get_depth_snapshot(const std::string& symbol, size_t depth_levels, 
                                    Timestamp timestamp = 0) const;
    
    // Trade history
    std::vector<TradeEvent> get_recent_trades(const std::string& symbol, size_t max_count) const;
    
    // Listener management
    void add_listener(std::shared_ptr<IMatchingEngineListener> listener);
    void remove_listener(std::shared_ptr<IMatchingEngineListener> listener);
    
    // Telemetry
    const Telemetry& get_telemetry() const { return telemetry_; }
    nlohmann::json get_telemetry_json() const { return telemetry_.to_json(); }
    
    // Event log (deterministic mode)
    EventLog& get_event_log() { return event_log_; }
    void set_deterministic(bool enabled);
    bool is_deterministic() const { return event_log_.is_deterministic(); }
    
private:
    std::unordered_map<std::string, SymbolConfig> symbol_configs_;
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> order_books_;
    std::unordered_map<std::string, std::unique_ptr<TradeTape>> trade_tapes_;
    
    std::vector<std::shared_ptr<IMatchingEngineListener>> listeners_;
    
    EventLog event_log_;
    Telemetry telemetry_;
    
    uint64_t sequence_number_ = 0;
    
    // Validation
    ResultCode validate_new_order(const NewOrderRequest& request) const;
    ResultCode validate_cancel(const CancelRequest& request) const;
    ResultCode validate_replace(const ReplaceRequest& request) const;
    
    // Event notification
    void notify_order_accepted(const OrderAcceptedEvent& event);
    void notify_order_rejected(const OrderRejectedEvent& event);
    void notify_order_cancelled(const OrderCancelledEvent& event);
    void notify_order_replaced(const OrderReplacedEvent& event);
    void notify_trade(const TradeEvent& event);
    
    // Timing
    Timestamp get_timestamp() const;
    uint64_t get_sequence_number() { return ++sequence_number_; }
};

} // namespace lob

