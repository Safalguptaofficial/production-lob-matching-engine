#include "lob/matching_engine.hpp"
#include <chrono>

namespace lob {

MatchingEngine::MatchingEngine() : MatchingEngine(false) {}

MatchingEngine::MatchingEngine(bool deterministic) {
    event_log_.set_deterministic(deterministic);
    if (deterministic) {
        event_log_.set_log_path("logs/events.log");
    }
}

bool MatchingEngine::add_symbol(const SymbolConfig& config) {
    if (!config.is_valid()) {
        return false;
    }
    
    if (symbol_configs_.find(config.symbol) != symbol_configs_.end()) {
        return false;  // Symbol already exists
    }
    
    symbol_configs_[config.symbol] = config;
    order_books_[config.symbol] = std::make_unique<OrderBook>(config.symbol, config.stp_policy);
    trade_tapes_[config.symbol] = std::make_unique<TradeTape>();
    
    return true;
}

bool MatchingEngine::has_symbol(const std::string& symbol) const {
    return symbol_configs_.find(symbol) != symbol_configs_.end();
}

OrderResponse MatchingEngine::handle(const NewOrderRequest& request) {
    auto start_time = std::chrono::steady_clock::now();
    
    telemetry_.record_order_processed();
    
    // Log incoming message (deterministic mode)
    event_log_.log_new_order(request);
    
    OrderResponse response;
    response.order_id = request.order_id;
    
    // Validate
    ResultCode validation = validate_new_order(request);
    if (validation != ResultCode::SUCCESS) {
        telemetry_.record_order_rejected();
        
        response.result = validation;
        response.message = result_code_to_string(validation);
        
        // Create rejection event
        OrderRejectedEvent reject_event;
        reject_event.order_id = request.order_id;
        reject_event.symbol = request.symbol;
        reject_event.reason = validation;
        reject_event.message = response.message;
        reject_event.timestamp = get_timestamp();
        reject_event.sequence_number = get_sequence_number();
        
        response.rejects.push_back(reject_event);
        notify_order_rejected(reject_event);
        event_log_.log_event(reject_event);
        
        return response;
    }
    
    // Route to order book
    Order order = request.to_order();
    auto& book = order_books_[request.symbol];
    std::vector<TradeEvent> trades = book->add_order(order);
    
    // Record acceptance
    telemetry_.record_order_accepted();
    
    OrderAcceptedEvent accept_event;
    accept_event.order_id = request.order_id;
    accept_event.symbol = request.symbol;
    accept_event.side = request.side;
    accept_event.price = request.price;
    accept_event.quantity = request.quantity;
    accept_event.timestamp = get_timestamp();
    accept_event.sequence_number = get_sequence_number();
    
    response.accepts.push_back(accept_event);
    notify_order_accepted(accept_event);
    event_log_.log_event(accept_event);
    
    // Process trades
    for (auto& trade : trades) {
        trade.sequence_number = get_sequence_number();
        response.trades.push_back(trade);
        
        telemetry_.record_trade(trade.symbol, trade.quantity);
        trade_tapes_[trade.symbol]->add_trade(trade);
        
        notify_trade(trade);
        event_log_.log_event(trade);
    }
    
    // Update symbol stats
    telemetry_.update_symbol_stats(request.symbol, book->get_stats());
    
    // Record latency
    auto end_time = std::chrono::steady_clock::now();
    auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    telemetry_.record_latency(latency_ns);
    
    response.result = ResultCode::SUCCESS;
    return response;
}

OrderResponse MatchingEngine::handle(const CancelRequest& request) {
    telemetry_.record_order_processed();
    
    event_log_.log_cancel(request);
    
    OrderResponse response;
    response.order_id = request.order_id;
    
    // Validate
    ResultCode validation = validate_cancel(request);
    if (validation != ResultCode::SUCCESS) {
        telemetry_.record_order_rejected();
        
        response.result = validation;
        response.message = result_code_to_string(validation);
        
        OrderRejectedEvent reject_event;
        reject_event.order_id = request.order_id;
        reject_event.symbol = request.symbol;
        reject_event.reason = validation;
        reject_event.message = response.message;
        reject_event.timestamp = get_timestamp();
        reject_event.sequence_number = get_sequence_number();
        
        response.rejects.push_back(reject_event);
        notify_order_rejected(reject_event);
        event_log_.log_event(reject_event);
        
        return response;
    }
    
    // Cancel order
    auto& book = order_books_[request.symbol];
    bool cancelled = book->cancel_order(request.order_id);
    
    if (cancelled) {
        telemetry_.record_order_cancelled();
        
        OrderCancelledEvent cancel_event;
        cancel_event.order_id = request.order_id;
        cancel_event.symbol = request.symbol;
        cancel_event.timestamp = get_timestamp();
        cancel_event.sequence_number = get_sequence_number();
        
        response.cancels.push_back(cancel_event);
        notify_order_cancelled(cancel_event);
        event_log_.log_event(cancel_event);
        
        response.result = ResultCode::SUCCESS;
    } else {
        response.result = ResultCode::REJECTED_ORDER_NOT_FOUND;
        response.message = "Order not found";
    }
    
    return response;
}

OrderResponse MatchingEngine::handle(const ReplaceRequest& request) {
    telemetry_.record_order_processed();
    
    event_log_.log_replace(request);
    
    OrderResponse response;
    response.order_id = request.order_id;
    
    // Validate
    ResultCode validation = validate_replace(request);
    if (validation != ResultCode::SUCCESS) {
        telemetry_.record_order_rejected();
        
        response.result = validation;
        response.message = result_code_to_string(validation);
        
        return response;
    }
    
    // Replace order (cancel + re-add)
    auto& book = order_books_[request.symbol];
    std::vector<TradeEvent> trades =
        book->replace_order(request.order_id, request.new_price, request.new_quantity);
    
    // Record replacement
    OrderReplacedEvent replace_event;
    replace_event.old_order_id = request.order_id;
    replace_event.new_order_id = request.order_id;  // Same ID for simplicity
    replace_event.symbol = request.symbol;
    replace_event.new_price = request.new_price;
    replace_event.new_quantity = request.new_quantity;
    replace_event.timestamp = get_timestamp();
    replace_event.sequence_number = get_sequence_number();
    
    response.replaces.push_back(replace_event);
    notify_order_replaced(replace_event);
    event_log_.log_event(replace_event);
    
    // Process trades (if any due to replacement)
    for (auto& trade : trades) {
        trade.sequence_number = get_sequence_number();
        response.trades.push_back(trade);
        
        telemetry_.record_trade(trade.symbol, trade.quantity);
        trade_tapes_[trade.symbol]->add_trade(trade);
        
        notify_trade(trade);
        event_log_.log_event(trade);
    }
    
    response.result = ResultCode::SUCCESS;
    return response;
}

TopOfBook MatchingEngine::get_top_of_book(const std::string& symbol, Timestamp timestamp) const {
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        return TopOfBook{};
    }
    
    if (timestamp == 0) {
        timestamp = get_timestamp();
    }
    
    return it->second->get_top_of_book(timestamp);
}

DepthSnapshot MatchingEngine::get_depth_snapshot(const std::string& symbol, size_t depth_levels,
                                                  Timestamp timestamp) const {
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        return DepthSnapshot{};
    }
    
    if (timestamp == 0) {
        timestamp = get_timestamp();
    }
    
    return it->second->get_depth_snapshot(depth_levels, timestamp);
}

std::vector<TradeEvent> MatchingEngine::get_recent_trades(const std::string& symbol,
                                                           size_t max_count) const {
    auto it = trade_tapes_.find(symbol);
    if (it == trade_tapes_.end()) {
        return {};
    }
    
    return it->second->get_recent_trades(max_count);
}

void MatchingEngine::add_listener(std::shared_ptr<IMatchingEngineListener> listener) {
    listeners_.push_back(listener);
}

void MatchingEngine::remove_listener(std::shared_ptr<IMatchingEngineListener> listener) {
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
}

void MatchingEngine::set_deterministic(bool enabled) {
    event_log_.set_deterministic(enabled);
}

// ============================================================================
// Private Methods: Validation
// ============================================================================

ResultCode MatchingEngine::validate_new_order(const NewOrderRequest& request) const {
    if (!has_symbol(request.symbol)) {
        return ResultCode::REJECTED_INVALID_SYMBOL;
    }
    
    if (request.order_type == OrderType::LIMIT && request.price <= 0) {
        return ResultCode::REJECTED_INVALID_PRICE;
    }
    
    if (request.quantity == 0) {
        return ResultCode::REJECTED_INVALID_QUANTITY;
    }
    
    return ResultCode::SUCCESS;
}

ResultCode MatchingEngine::validate_cancel(const CancelRequest& request) const {
    if (!has_symbol(request.symbol)) {
        return ResultCode::REJECTED_INVALID_SYMBOL;
    }
    
    return ResultCode::SUCCESS;
}

ResultCode MatchingEngine::validate_replace(const ReplaceRequest& request) const {
    if (!has_symbol(request.symbol)) {
        return ResultCode::REJECTED_INVALID_SYMBOL;
    }
    
    if (request.new_price <= 0) {
        return ResultCode::REJECTED_INVALID_PRICE;
    }
    
    if (request.new_quantity == 0) {
        return ResultCode::REJECTED_INVALID_QUANTITY;
    }
    
    return ResultCode::SUCCESS;
}

// ============================================================================
// Private Methods: Event Notification
// ============================================================================

void MatchingEngine::notify_order_accepted(const OrderAcceptedEvent& event) {
    for (auto& listener : listeners_) {
        listener->on_order_accepted(event);
    }
}

void MatchingEngine::notify_order_rejected(const OrderRejectedEvent& event) {
    for (auto& listener : listeners_) {
        listener->on_order_rejected(event);
    }
}

void MatchingEngine::notify_order_cancelled(const OrderCancelledEvent& event) {
    for (auto& listener : listeners_) {
        listener->on_order_cancelled(event);
    }
}

void MatchingEngine::notify_order_replaced(const OrderReplacedEvent& event) {
    for (auto& listener : listeners_) {
        listener->on_order_replaced(event);
    }
}

void MatchingEngine::notify_trade(const TradeEvent& event) {
    for (auto& listener : listeners_) {
        listener->on_trade(event);
    }
}

// ============================================================================
// Private Methods: Utilities
// ============================================================================

Timestamp MatchingEngine::get_timestamp() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

} // namespace lob

