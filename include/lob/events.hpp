#pragma once

#include "types.hpp"
#include <string>

namespace lob {

// Trade event generated when orders match
struct TradeEvent {
    TradeId trade_id = 0;
    std::string symbol;
    Price price = INVALID_PRICE;
    Quantity quantity = INVALID_QUANTITY;
    Side aggressor_side = Side::BUY;
    OrderId aggressive_order_id = INVALID_ORDER_ID;
    OrderId passive_order_id = INVALID_ORDER_ID;
    TraderId aggressive_trader_id = INVALID_TRADER_ID;
    TraderId passive_trader_id = INVALID_TRADER_ID;
    Timestamp timestamp = 0;
    uint64_t sequence_number = 0;
};

// Order accepted event
struct OrderAcceptedEvent {
    OrderId order_id = INVALID_ORDER_ID;
    std::string symbol;
    Side side = Side::BUY;
    Price price = INVALID_PRICE;
    Quantity quantity = INVALID_QUANTITY;
    Timestamp timestamp = 0;
    uint64_t sequence_number = 0;
};

// Order rejected event
struct OrderRejectedEvent {
    OrderId order_id = INVALID_ORDER_ID;
    std::string symbol;
    ResultCode reason = ResultCode::REJECTED_UNKNOWN_ERROR;
    std::string message;
    Timestamp timestamp = 0;
    uint64_t sequence_number = 0;
};

// Order cancelled event
struct OrderCancelledEvent {
    OrderId order_id = INVALID_ORDER_ID;
    std::string symbol;
    Quantity remaining_quantity = INVALID_QUANTITY;
    Timestamp timestamp = 0;
    uint64_t sequence_number = 0;
};

// Order replaced event
struct OrderReplacedEvent {
    OrderId old_order_id = INVALID_ORDER_ID;
    OrderId new_order_id = INVALID_ORDER_ID;
    std::string symbol;
    Price new_price = INVALID_PRICE;
    Quantity new_quantity = INVALID_QUANTITY;
    Timestamp timestamp = 0;
    uint64_t sequence_number = 0;
};

// Book update event (for market data feeds)
struct BookUpdateEvent {
    std::string symbol;
    Side side = Side::BUY;
    Price price = INVALID_PRICE;
    Quantity quantity = INVALID_QUANTITY; // 0 means price level removed
    Timestamp timestamp = 0;
    uint64_t sequence_number = 0;
};

} // namespace lob

