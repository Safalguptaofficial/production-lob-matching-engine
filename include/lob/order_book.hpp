#pragma once

#include "types.hpp"
#include "order.hpp"
#include "events.hpp"
#include "market_data.hpp"
#include "telemetry.hpp"
#include <map>
#include <deque>
#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>

namespace lob {

// Single price level with FIFO queue
class PriceLevelQueue {
public:
    void add_order(Order* order);
    void remove_order(Order* order);
    
    Quantity total_quantity() const { return total_quantity_; }
    size_t order_count() const { return orders_.size(); }
    bool empty() const { return orders_.empty(); }
    
    const std::deque<Order*>& orders() const { return orders_; }
    
    Order* front() { return orders_.empty() ? nullptr : orders_.front(); }
    void pop_front() { orders_.pop_front(); }
    
private:
    std::deque<Order*> orders_;
    Quantity total_quantity_ = 0;
};

// Order book for a single symbol
class OrderBook {
public:
    explicit OrderBook(const std::string& symbol, STPPolicy stp_policy = STPPolicy::CANCEL_INCOMING);
    ~OrderBook();
    
    // Order operations
    std::vector<TradeEvent> add_order(const Order& order);
    bool cancel_order(OrderId order_id);
    std::vector<TradeEvent> replace_order(OrderId order_id, Price new_price, Quantity new_quantity);
    
    // Query operations
    std::optional<Price> get_best_bid() const;
    std::optional<Price> get_best_ask() const;
    
    TopOfBook get_top_of_book(Timestamp timestamp) const;
    DepthSnapshot get_depth_snapshot(size_t depth_levels, Timestamp timestamp) const;
    
    const Order* find_order(OrderId order_id) const;
    
    // Statistics
    size_t active_order_count() const { return orders_.size(); }
    size_t bid_level_count() const { return bids_.size(); }
    size_t ask_level_count() const { return asks_.size(); }
    
    SymbolStats get_stats() const;
    
    const std::string& symbol() const { return symbol_; }
    
private:
    std::string symbol_;
    STPPolicy stp_policy_;
    
    // Price levels: price -> FIFO queue
    std::map<Price, PriceLevelQueue, std::greater<Price>> bids_; // Descending (best bid first)
    std::map<Price, PriceLevelQueue, std::less<Price>> asks_;    // Ascending (best ask first)
    
    // Order lookup: order_id -> Order*
    std::unordered_map<OrderId, std::unique_ptr<Order>> orders_;
    
    // Trade ID generator
    TradeId next_trade_id_ = 1;
    uint64_t trade_count_ = 0;
    uint64_t total_volume_ = 0;
    
    // Matching logic
    std::vector<TradeEvent> match_order(Order* order, Timestamp timestamp);
    std::vector<TradeEvent> match_limit_order(Order* order, Timestamp timestamp);
    std::vector<TradeEvent> match_market_order(Order* order, Timestamp timestamp);
    
    // Check if order would self-trade
    bool would_self_trade(const Order* incoming, const Order* resting) const;
    
    // Handle self-trade
    void handle_self_trade(Order* incoming, Order* resting, std::vector<TradeEvent>& trades);
    
    // Add order to book (after matching)
    void add_to_book(Order* order);
    
    // Remove order from book
    void remove_from_book(Order* order);
    
    // Create trade event
    TradeEvent create_trade(Order* aggressive, Order* passive, Quantity quantity, 
                           Price price, Timestamp timestamp);
};

} // namespace lob

