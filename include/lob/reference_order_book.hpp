#pragma once

#include "types.hpp"
#include "order.hpp"
#include "events.hpp"
#include "market_data.hpp"
#include <vector>
#include <memory>
#include <optional>

namespace lob {

// Naive O(N²) reference implementation for validation
// Deliberately simple and easy to verify for correctness
class ReferenceOrderBook {
public:
    explicit ReferenceOrderBook(const std::string& symbol, 
                               STPPolicy stp_policy = STPPolicy::CANCEL_INCOMING);
    
    // Same interface as optimized OrderBook
    std::vector<TradeEvent> add_order(const Order& order);
    bool cancel_order(OrderId order_id);
    std::vector<TradeEvent> replace_order(OrderId order_id, Price new_price, Quantity new_quantity);
    
    // Query operations
    std::optional<Price> get_best_bid() const;
    std::optional<Price> get_best_ask() const;
    
    TopOfBook get_top_of_book(Timestamp timestamp) const;
    DepthSnapshot get_depth_snapshot(size_t depth_levels, Timestamp timestamp) const;
    
    const Order* find_order(OrderId order_id) const;
    
    const std::string& symbol() const { return symbol_; }
    
    // Access all orders (for validation)
    const std::vector<std::unique_ptr<Order>>& all_orders() const { return orders_; }
    
private:
    std::string symbol_;
    STPPolicy stp_policy_;
    
    // Simple vector of all orders (linear search)
    std::vector<std::unique_ptr<Order>> orders_;
    
    TradeId next_trade_id_ = 1;
    
    // Naive matching: linear search for best matching order
    std::vector<TradeEvent> match_order(Order* order, Timestamp timestamp);
    
    // Find best matching order (linear search)
    Order* find_best_match(const Order* incoming);
    
    // Check if orders can trade
    bool can_trade(const Order* incoming, const Order* resting) const;
    
    // Check self-trade
    bool would_self_trade(const Order* incoming, const Order* resting) const;
    
    // Create trade event
    TradeEvent create_trade(Order* aggressive, Order* passive, Quantity quantity,
                           Price price, Timestamp timestamp);
};

} // namespace lob

