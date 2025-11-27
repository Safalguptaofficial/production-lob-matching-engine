#include "lob/order_book.hpp"
#include <algorithm>
#include <chrono>

namespace lob {

// ============================================================================
// PriceLevelQueue Implementation
// ============================================================================

void PriceLevelQueue::add_order(Order* order) {
    orders_.push_back(order);
    total_quantity_ += order->remaining_quantity;
}

void PriceLevelQueue::remove_order(Order* order) {
    auto it = std::find(orders_.begin(), orders_.end(), order);
    if (it != orders_.end()) {
        total_quantity_ -= (*it)->remaining_quantity;
        orders_.erase(it);
    }
}

// ============================================================================
// OrderBook Implementation
// ============================================================================

OrderBook::OrderBook(const std::string& symbol, STPPolicy stp_policy)
    : symbol_(symbol), stp_policy_(stp_policy) {}

OrderBook::~OrderBook() = default;

std::vector<TradeEvent> OrderBook::add_order(const Order& order) {
    // Create a copy of the order and store it
    auto order_ptr = std::make_unique<Order>(order);
    Order* order_raw = order_ptr.get();
    
    // Get current timestamp for trades
    auto now = std::chrono::steady_clock::now();
    Timestamp timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             now.time_since_epoch())
                             .count();
    
    // Try to match the order
    std::vector<TradeEvent> trades = match_order(order_raw, timestamp);
    
    // If order still has remaining quantity and should rest on book
    if (order_raw->remaining_quantity > 0) {
        // Check time in force
        if (order_raw->is_ioc()) {
            // IOC: Don't add to book, just return trades
            return trades;
        } else if (order_raw->is_fok()) {
            // FOK: Should have been fully filled, return empty (order rejected)
            return {};
        } else {
            // DAY/GTC: Add to book
            orders_[order_raw->order_id] = std::move(order_ptr);
            add_to_book(order_raw);
        }
    } else {
        // Fully filled, don't add to book
        // order_ptr will be destroyed (not added to orders_ map)
    }
    
    return trades;
}

bool OrderBook::cancel_order(OrderId order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;  // Order not found
    }
    
    Order* order = it->second.get();
    remove_from_book(order);
    orders_.erase(it);
    
    return true;
}

std::vector<TradeEvent> OrderBook::replace_order(OrderId order_id, Price new_price,
                                                  Quantity new_quantity) {
    // Find existing order
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return {};  // Order not found
    }
    
    Order* old_order = it->second.get();
    
    // Cancel old order
    remove_from_book(old_order);
    
    // Create new order with same properties but new price/quantity
    Order new_order = *old_order;
    new_order.price = new_price;
    new_order.quantity = new_quantity;
    new_order.remaining_quantity = new_quantity;
    
    // Remove old order from map
    orders_.erase(it);
    
    // Add new order (which will try to match)
    return add_order(new_order);
}

std::optional<Price> OrderBook::get_best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::get_best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

TopOfBook OrderBook::get_top_of_book(Timestamp timestamp) const {
    TopOfBook tob;
    tob.symbol = symbol_;
    tob.timestamp = timestamp;
    
    if (!bids_.empty()) {
        tob.best_bid = bids_.begin()->first;
        tob.bid_size = bids_.begin()->second.total_quantity();
    }
    
    if (!asks_.empty()) {
        tob.best_ask = asks_.begin()->first;
        tob.ask_size = asks_.begin()->second.total_quantity();
    }
    
    return tob;
}

DepthSnapshot OrderBook::get_depth_snapshot(size_t depth_levels, Timestamp timestamp) const {
    DepthSnapshot snapshot;
    snapshot.symbol = symbol_;
    snapshot.timestamp = timestamp;
    snapshot.sequence_number = trade_count_;
    
    // Collect bid levels (descending price)
    size_t count = 0;
    for (const auto& [price, queue] : bids_) {
        if (count >= depth_levels) break;
        PriceLevel level;
        level.price = price;
        level.quantity = queue.total_quantity();
        level.order_count = queue.order_count();
        snapshot.bids.push_back(level);
        ++count;
    }
    
    // Collect ask levels (ascending price)
    count = 0;
    for (const auto& [price, queue] : asks_) {
        if (count >= depth_levels) break;
        PriceLevel level;
        level.price = price;
        level.quantity = queue.total_quantity();
        level.order_count = queue.order_count();
        snapshot.asks.push_back(level);
        ++count;
    }
    
    return snapshot;
}

const Order* OrderBook::find_order(OrderId order_id) const {
    auto it = orders_.find(order_id);
    return it != orders_.end() ? it->second.get() : nullptr;
}

SymbolStats OrderBook::get_stats() const {
    SymbolStats stats;
    stats.active_orders = orders_.size();
    stats.bid_levels = bids_.size();
    stats.ask_levels = asks_.size();
    stats.trade_count = trade_count_;
    stats.trade_volume = total_volume_;
    
    auto best_bid = get_best_bid();
    auto best_ask = get_best_ask();
    stats.best_bid = best_bid.value_or(INVALID_PRICE);
    stats.best_ask = best_ask.value_or(INVALID_PRICE);
    
    // Calculate max depth
    for (const auto& [price, queue] : bids_) {
        stats.max_bid_depth = std::max(stats.max_bid_depth, queue.total_quantity());
    }
    for (const auto& [price, queue] : asks_) {
        stats.max_ask_depth = std::max(stats.max_ask_depth, queue.total_quantity());
    }
    
    return stats;
}

// ============================================================================
// Private Methods: Matching Logic
// ============================================================================

std::vector<TradeEvent> OrderBook::match_order(Order* order, Timestamp timestamp) {
    if (order->is_limit()) {
        return match_limit_order(order, timestamp);
    } else if (order->is_market()) {
        return match_market_order(order, timestamp);
    }
    return {};
}

std::vector<TradeEvent> OrderBook::match_limit_order(Order* order, Timestamp timestamp) {
    std::vector<TradeEvent> trades;
    
    // Match against opposite side
    if (order->is_buy()) {
        // Buy order matches against asks (ascending)
        while (order->remaining_quantity > 0 && !asks_.empty()) {
            auto& [price, queue] = *asks_.begin();
            
            // Check if prices cross
            if (order->price < price) {
                break;  // No more matches possible
            }
            
            // Match against orders in FIFO queue
            while (order->remaining_quantity > 0 && !queue.empty()) {
                Order* resting_order = queue.front();
                
                // Check for self-trade
                if (would_self_trade(order, resting_order)) {
                    handle_self_trade(order, resting_order, trades);
                    if (order->remaining_quantity == 0) break;
                    continue;
                }
                
                // Calculate trade quantity
                Quantity trade_qty =
                    std::min(order->remaining_quantity, resting_order->remaining_quantity);
                
                // Create trade event
                TradeEvent trade = create_trade(order, resting_order, trade_qty, price, timestamp);
                trades.push_back(trade);
                
                // Update quantities
                order->remaining_quantity -= trade_qty;
                resting_order->remaining_quantity -= trade_qty;
                
                // Update statistics
                ++trade_count_;
                total_volume_ += trade_qty;
                
                // Remove fully filled resting order
                if (resting_order->remaining_quantity == 0) {
                    queue.pop_front();
                    orders_.erase(resting_order->order_id);
                }
            }
            
            // Remove empty price level
            if (queue.empty()) {
                asks_.erase(asks_.begin());
            }
        }
    } else {
        // Sell order matches against bids (descending)
        while (order->remaining_quantity > 0 && !bids_.empty()) {
            auto& [price, queue] = *bids_.begin();
            
            // Check if prices cross
            if (order->price > price) {
                break;  // No more matches possible
            }
            
            // Match against orders in FIFO queue
            while (order->remaining_quantity > 0 && !queue.empty()) {
                Order* resting_order = queue.front();
                
                // Check for self-trade
                if (would_self_trade(order, resting_order)) {
                    handle_self_trade(order, resting_order, trades);
                    if (order->remaining_quantity == 0) break;
                    continue;
                }
                
                // Calculate trade quantity
                Quantity trade_qty =
                    std::min(order->remaining_quantity, resting_order->remaining_quantity);
                
                // Create trade event
                TradeEvent trade = create_trade(order, resting_order, trade_qty, price, timestamp);
                trades.push_back(trade);
                
                // Update quantities
                order->remaining_quantity -= trade_qty;
                resting_order->remaining_quantity -= trade_qty;
                
                // Update statistics
                ++trade_count_;
                total_volume_ += trade_qty;
                
                // Remove fully filled resting order
                if (resting_order->remaining_quantity == 0) {
                    queue.pop_front();
                    orders_.erase(resting_order->order_id);
                }
            }
            
            // Remove empty price level
            if (queue.empty()) {
                bids_.erase(bids_.begin());
            }
        }
    }
    
    return trades;
}

std::vector<TradeEvent> OrderBook::match_market_order(Order* order, Timestamp timestamp) {
    std::vector<TradeEvent> trades;
    
    // Market order matches at any price until filled or book exhausted
    if (order->is_buy()) {
        // Buy market order matches against asks
        while (order->remaining_quantity > 0 && !asks_.empty()) {
            auto& [price, queue] = *asks_.begin();
            
            // Match against orders in FIFO queue
            while (order->remaining_quantity > 0 && !queue.empty()) {
                Order* resting_order = queue.front();
                
                // Check for self-trade
                if (would_self_trade(order, resting_order)) {
                    handle_self_trade(order, resting_order, trades);
                    if (order->remaining_quantity == 0) break;
                    continue;
                }
                
                // Calculate trade quantity
                Quantity trade_qty =
                    std::min(order->remaining_quantity, resting_order->remaining_quantity);
                
                // Create trade event (at resting order's price)
                TradeEvent trade = create_trade(order, resting_order, trade_qty, price, timestamp);
                trades.push_back(trade);
                
                // Update quantities
                order->remaining_quantity -= trade_qty;
                resting_order->remaining_quantity -= trade_qty;
                
                // Update statistics
                ++trade_count_;
                total_volume_ += trade_qty;
                
                // Remove fully filled resting order
                if (resting_order->remaining_quantity == 0) {
                    queue.pop_front();
                    orders_.erase(resting_order->order_id);
                }
            }
            
            // Remove empty price level
            if (queue.empty()) {
                asks_.erase(asks_.begin());
            }
        }
    } else {
        // Sell market order matches against bids
        while (order->remaining_quantity > 0 && !bids_.empty()) {
            auto& [price, queue] = *bids_.begin();
            
            // Match against orders in FIFO queue
            while (order->remaining_quantity > 0 && !queue.empty()) {
                Order* resting_order = queue.front();
                
                // Check for self-trade
                if (would_self_trade(order, resting_order)) {
                    handle_self_trade(order, resting_order, trades);
                    if (order->remaining_quantity == 0) break;
                    continue;
                }
                
                // Calculate trade quantity
                Quantity trade_qty =
                    std::min(order->remaining_quantity, resting_order->remaining_quantity);
                
                // Create trade event (at resting order's price)
                TradeEvent trade = create_trade(order, resting_order, trade_qty, price, timestamp);
                trades.push_back(trade);
                
                // Update quantities
                order->remaining_quantity -= trade_qty;
                resting_order->remaining_quantity -= trade_qty;
                
                // Update statistics
                ++trade_count_;
                total_volume_ += trade_qty;
                
                // Remove fully filled resting order
                if (resting_order->remaining_quantity == 0) {
                    queue.pop_front();
                    orders_.erase(resting_order->order_id);
                }
            }
            
            // Remove empty price level
            if (queue.empty()) {
                bids_.erase(bids_.begin());
            }
        }
    }
    
    return trades;
}

bool OrderBook::would_self_trade(const Order* incoming, const Order* resting) const {
    if (stp_policy_ == STPPolicy::NONE) {
        return false;
    }
    return incoming->trader_id == resting->trader_id && incoming->trader_id != INVALID_TRADER_ID;
}

void OrderBook::handle_self_trade(Order* incoming, Order* resting,
                                  std::vector<TradeEvent>& /*trades*/) {
    // Simple implementation: cancel incoming (CANCEL_INCOMING policy)
    // For now, just skip this match by removing the resting order from queue
    // In a full implementation, we'd apply the configured policy
    
    if (stp_policy_ == STPPolicy::CANCEL_INCOMING) {
        // Stop matching for incoming order
        incoming->remaining_quantity = 0;
    } else if (stp_policy_ == STPPolicy::CANCEL_RESTING) {
        // Remove resting order
        remove_from_book(resting);
        orders_.erase(resting->order_id);
    } else if (stp_policy_ == STPPolicy::CANCEL_BOTH) {
        // Remove both
        incoming->remaining_quantity = 0;
        remove_from_book(resting);
        orders_.erase(resting->order_id);
    }
}

void OrderBook::add_to_book(Order* order) {
    if (order->is_buy()) {
        bids_[order->price].add_order(order);
    } else {
        asks_[order->price].add_order(order);
    }
}

void OrderBook::remove_from_book(Order* order) {
    if (order->is_buy()) {
        auto it = bids_.find(order->price);
        if (it != bids_.end()) {
            it->second.remove_order(order);
            if (it->second.empty()) {
                bids_.erase(it);
            }
        }
    } else {
        auto it = asks_.find(order->price);
        if (it != asks_.end()) {
            it->second.remove_order(order);
            if (it->second.empty()) {
                asks_.erase(it);
            }
        }
    }
}

TradeEvent OrderBook::create_trade(Order* aggressive, Order* passive, Quantity quantity,
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
    trade.sequence_number = trade_count_;
    
    return trade;
}

} // namespace lob

