#pragma once

#include "types.hpp"
#include <string>

namespace lob {

// Order structure with all required fields
struct Order {
    OrderId order_id = INVALID_ORDER_ID;
    TraderId trader_id = INVALID_TRADER_ID;
    std::string symbol;
    Side side = Side::BUY;
    OrderType order_type = OrderType::LIMIT;
    Price price = INVALID_PRICE;
    Quantity quantity = INVALID_QUANTITY;
    Quantity remaining_quantity = INVALID_QUANTITY;
    TimeInForce time_in_force = TimeInForce::DAY;
    Timestamp timestamp = 0;
    
    // Advanced flags (for future use)
    bool post_only = false;
    bool hidden = false;
    Quantity display_quantity = 0; // For iceberg orders (0 = show all)
    
    // Helper methods
    bool is_buy() const { return side == Side::BUY; }
    bool is_sell() const { return side == Side::SELL; }
    bool is_limit() const { return order_type == OrderType::LIMIT; }
    bool is_market() const { return order_type == OrderType::MARKET; }
    bool is_fully_filled() const { return remaining_quantity == 0; }
    bool is_ioc() const { return time_in_force == TimeInForce::IOC; }
    bool is_fok() const { return time_in_force == TimeInForce::FOK; }
    
    Quantity filled_quantity() const { return quantity - remaining_quantity; }
};

} // namespace lob

