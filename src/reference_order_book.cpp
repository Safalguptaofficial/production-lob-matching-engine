#include "lob/reference_order_book.hpp"
#include <algorithm>
#include <chrono>

namespace lob {

ReferenceOrderBook::ReferenceOrderBook(const std::string& symbol, STPPolicy stp_policy)
    : symbol_(symbol), stp_policy_(stp_policy) {}

std::vector<TradeEvent> ReferenceOrderBook::add_order(const Order& order) {
    auto order_ptr = std::make_unique<Order>(order);
    Order* order_raw = order_ptr.get();
    
    auto now = std::chrono::steady_clock::now();
    Timestamp timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             now.time_since_epoch())
                             .count();
    
    // Try to match
    std::vector<TradeEvent> trades = match_order(order_raw, timestamp);
    
    // If order has remaining quantity and should rest on book
    if (order_raw->remaining_quantity > 0) {
        if (order_raw->is_ioc()) {
            // IOC: don't add to book
            return trades;
        } else if (order_raw->is_fok()) {
            // FOK: should have been fully filled
            return {};
        } else {
            // Add to book
            orders_.push_back(std::move(order_ptr));
        }
    }
    
    return trades;
}

bool ReferenceOrderBook::cancel_order(OrderId order_id) {
    auto it = std::find_if(orders_.begin(), orders_.end(),
                          [order_id](const auto& order) { return order->order_id == order_id; });
    
    if (it != orders_.end()) {
        orders_.erase(it);
        return true;
    }
    
    return false;
}

std::vector<TradeEvent> ReferenceOrderBook::replace_order(OrderId order_id, Price new_price,
                                                           Quantity new_quantity) {
    // Cancel old order
    if (!cancel_order(order_id)) {
        return {};
    }
    
    // Create new order (reusing order_id for simplicity)
    Order new_order;
    new_order.order_id = order_id;
    new_order.price = new_price;
    new_order.quantity = new_quantity;
    new_order.remaining_quantity = new_quantity;
    
    return add_order(new_order);
}

std::optional<Price> ReferenceOrderBook::get_best_bid() const {
    Price best = INVALID_PRICE;
    
    for (const auto& order : orders_) {
        if (order->is_buy() && order->remaining_quantity > 0) {
            if (best == INVALID_PRICE || order->price > best) {
                best = order->price;
            }
        }
    }
    
    return best != INVALID_PRICE ? std::optional<Price>(best) : std::nullopt;
}

std::optional<Price> ReferenceOrderBook::get_best_ask() const {
    Price best = INVALID_PRICE;
    
    for (const auto& order : orders_) {
        if (order->is_sell() && order->remaining_quantity > 0) {
            if (best == INVALID_PRICE || order->price < best) {
                best = order->price;
            }
        }
    }
    
    return best != INVALID_PRICE ? std::optional<Price>(best) : std::nullopt;
}

TopOfBook ReferenceOrderBook::get_top_of_book(Timestamp timestamp) const {
    TopOfBook tob;
    tob.symbol = symbol_;
    tob.timestamp = timestamp;
    
    auto best_bid = get_best_bid();
    auto best_ask = get_best_ask();
    
    if (best_bid) {
        tob.best_bid = *best_bid;
        // Calculate bid size
        for (const auto& order : orders_) {
            if (order->is_buy() && order->price == *best_bid) {
                tob.bid_size += order->remaining_quantity;
            }
        }
    }
    
    if (best_ask) {
        tob.best_ask = *best_ask;
        // Calculate ask size
        for (const auto& order : orders_) {
            if (order->is_sell() && order->price == *best_ask) {
                tob.ask_size += order->remaining_quantity;
            }
        }
    }
    
    return tob;
}

DepthSnapshot ReferenceOrderBook::get_depth_snapshot(size_t depth_levels,
                                                     Timestamp timestamp) const {
    DepthSnapshot snapshot;
    snapshot.symbol = symbol_;
    snapshot.timestamp = timestamp;
    
    // This is a naive implementation - just get unique prices and aggregate
    // In practice, would be more efficient but this is deliberately simple
    
    // Collect all unique bid prices
    std::map<Price, Quantity, std::greater<Price>> bid_map;
    for (const auto& order : orders_) {
        if (order->is_buy() && order->remaining_quantity > 0) {
            bid_map[order->price] += order->remaining_quantity;
        }
    }
    
    // Collect all unique ask prices
    std::map<Price, Quantity, std::less<Price>> ask_map;
    for (const auto& order : orders_) {
        if (order->is_sell() && order->remaining_quantity > 0) {
            ask_map[order->price] += order->remaining_quantity;
        }
    }
    
    // Build bid levels
    size_t count = 0;
    for (const auto& [price, qty] : bid_map) {
        if (count >= depth_levels) break;
        PriceLevel level;
        level.price = price;
        level.quantity = qty;
        level.order_count = 1;  // Simplified
        snapshot.bids.push_back(level);
        ++count;
    }
    
    // Build ask levels
    count = 0;
    for (const auto& [price, qty] : ask_map) {
        if (count >= depth_levels) break;
        PriceLevel level;
        level.price = price;
        level.quantity = qty;
        level.order_count = 1;  // Simplified
        snapshot.asks.push_back(level);
        ++count;
    }
    
    return snapshot;
}

const Order* ReferenceOrderBook::find_order(OrderId order_id) const {
    auto it = std::find_if(orders_.begin(), orders_.end(),
                          [order_id](const auto& order) { return order->order_id == order_id; });
    
    return it != orders_.end() ? it->get() : nullptr;
}

// ============================================================================
// Private Methods: Matching Logic (Naive)
// ============================================================================

std::vector<TradeEvent> ReferenceOrderBook::match_order(Order* incoming, Timestamp timestamp) {
    std::vector<TradeEvent> trades;
    
    while (incoming->remaining_quantity > 0) {
        // Find best matching order (linear search)
        Order* best_match = find_best_match(incoming);
        
        if (!best_match) {
            break;  // No more matches
        }
        
        // Check self-trade
        if (would_self_trade(incoming, best_match)) {
            if (stp_policy_ == STPPolicy::CANCEL_INCOMING) {
                incoming->remaining_quantity = 0;
                break;
            } else if (stp_policy_ == STPPolicy::CANCEL_RESTING) {
                cancel_order(best_match->order_id);
                continue;
            } else if (stp_policy_ == STPPolicy::CANCEL_BOTH) {
                incoming->remaining_quantity = 0;
                cancel_order(best_match->order_id);
                break;
            }
        }
        
        // Calculate trade quantity
        Quantity trade_qty = std::min(incoming->remaining_quantity, best_match->remaining_quantity);
        
        // Create trade
        TradeEvent trade = create_trade(incoming, best_match, trade_qty, best_match->price, timestamp);
        trades.push_back(trade);
        
        // Update quantities
        incoming->remaining_quantity -= trade_qty;
        best_match->remaining_quantity -= trade_qty;
        
        // Remove fully filled order
        if (best_match->remaining_quantity == 0) {
            cancel_order(best_match->order_id);
        }
    }
    
    return trades;
}

Order* ReferenceOrderBook::find_best_match(const Order* incoming) {
    Order* best = nullptr;
    
    // Linear search for best matching order
    for (auto& order_ptr : orders_) {
        Order* order = order_ptr.get();
        
        // Skip if same side or no quantity
        if (order->side == incoming->side || order->remaining_quantity == 0) {
            continue;
        }
        
        // Check if can trade
        if (!can_trade(incoming, order)) {
            continue;
        }
        
        // First match or better price/time
        if (!best) {
            best = order;
        } else {
            // Better price
            if (incoming->is_buy()) {
                if (order->price < best->price) {
                    best = order;
                } else if (order->price == best->price && order->timestamp < best->timestamp) {
                    best = order;
                }
            } else {
                if (order->price > best->price) {
                    best = order;
                } else if (order->price == best->price && order->timestamp < best->timestamp) {
                    best = order;
                }
            }
        }
    }
    
    return best;
}

bool ReferenceOrderBook::can_trade(const Order* incoming, const Order* resting) const {
    if (incoming->is_market() || resting->is_market()) {
        return true;  // Market orders trade at any price
    }
    
    if (incoming->is_buy()) {
        return incoming->price >= resting->price;
    } else {
        return incoming->price <= resting->price;
    }
}

bool ReferenceOrderBook::would_self_trade(const Order* incoming, const Order* resting) const {
    if (stp_policy_ == STPPolicy::NONE) {
        return false;
    }
    return incoming->trader_id == resting->trader_id && incoming->trader_id != INVALID_TRADER_ID;
}

TradeEvent ReferenceOrderBook::create_trade(Order* aggressive, Order* passive, Quantity quantity,
                                            Price price, Timestamp timestamp) {
    TradeEvent trade;
    trade.trade_id = next_trade_id_++;
    trade.symbol = symbol_;
    trade.price = price;
    trade.quantity = quantity;
    trade.aggressor_side = aggressive->side;
    trade.aggressive_order_id = aggressive->order_id;
    trade.passive_order_id = passive->order_id;
    trade.aggressive_trader_id = aggressive->trader_id;
    trade.passive_trader_id = passive->trader_id;
    trade.timestamp = timestamp;
    
    return trade;
}

} // namespace lob

