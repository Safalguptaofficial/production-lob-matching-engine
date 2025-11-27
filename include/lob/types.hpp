#pragma once

#include <cstdint>
#include <string>

namespace lob {

// Fundamental types for the order book
using OrderId = uint64_t;
using TraderId = uint64_t;
using TradeId = uint64_t;
using Price = int64_t;      // Fixed-point price (e.g., cents or ticks)
using Quantity = uint64_t;
using Timestamp = uint64_t; // Nanoseconds since epoch or monotonic counter

// Order side
enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

// Time in force options
enum class TimeInForce : uint8_t {
    DAY = 0,    // Good for day
    IOC = 1,    // Immediate or cancel
    FOK = 2,    // Fill or kill
    GTC = 3,    // Good till cancelled
    GTD = 4     // Good till date (optional)
};

// Order type
enum class OrderType : uint8_t {
    LIMIT = 0,
    MARKET = 1
};

// Self-trade prevention policy
enum class STPPolicy : uint8_t {
    NONE = 0,           // No self-trade prevention
    CANCEL_INCOMING = 1, // Cancel incoming order
    CANCEL_RESTING = 2,  // Cancel resting order
    CANCEL_BOTH = 3      // Cancel both orders
};

// Result status codes
enum class ResultCode : uint8_t {
    SUCCESS = 0,
    REJECTED_INVALID_SYMBOL = 1,
    REJECTED_INVALID_PRICE = 2,
    REJECTED_INVALID_QUANTITY = 3,
    REJECTED_ORDER_NOT_FOUND = 4,
    REJECTED_SELF_TRADE = 5,
    REJECTED_FOK_NOT_FILLABLE = 6,
    REJECTED_RISK_LIMIT = 7,
    REJECTED_UNKNOWN_ERROR = 255
};

// Helper functions
inline std::string side_to_string(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

inline std::string tif_to_string(TimeInForce tif) {
    switch (tif) {
        case TimeInForce::DAY: return "DAY";
        case TimeInForce::IOC: return "IOC";
        case TimeInForce::FOK: return "FOK";
        case TimeInForce::GTC: return "GTC";
        case TimeInForce::GTD: return "GTD";
        default: return "UNKNOWN";
    }
}

inline std::string order_type_to_string(OrderType type) {
    return type == OrderType::LIMIT ? "LIMIT" : "MARKET";
}

inline std::string result_code_to_string(ResultCode code) {
    switch (code) {
        case ResultCode::SUCCESS: return "SUCCESS";
        case ResultCode::REJECTED_INVALID_SYMBOL: return "REJECTED_INVALID_SYMBOL";
        case ResultCode::REJECTED_INVALID_PRICE: return "REJECTED_INVALID_PRICE";
        case ResultCode::REJECTED_INVALID_QUANTITY: return "REJECTED_INVALID_QUANTITY";
        case ResultCode::REJECTED_ORDER_NOT_FOUND: return "REJECTED_ORDER_NOT_FOUND";
        case ResultCode::REJECTED_SELF_TRADE: return "REJECTED_SELF_TRADE";
        case ResultCode::REJECTED_FOK_NOT_FILLABLE: return "REJECTED_FOK_NOT_FILLABLE";
        case ResultCode::REJECTED_RISK_LIMIT: return "REJECTED_RISK_LIMIT";
        default: return "REJECTED_UNKNOWN_ERROR";
    }
}

// Sentinel values
constexpr Price INVALID_PRICE = -1;
constexpr Quantity INVALID_QUANTITY = 0;
constexpr OrderId INVALID_ORDER_ID = 0;
constexpr TraderId INVALID_TRADER_ID = 0;

} // namespace lob

