#pragma once

#include "types.hpp"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace lob {

// Single price level in depth snapshot
struct PriceLevel {
    Price price = INVALID_PRICE;
    Quantity quantity = 0;
    uint32_t order_count = 0;
};

// Top-of-book snapshot
struct TopOfBook {
    std::string symbol;
    Price best_bid = INVALID_PRICE;
    Price best_ask = INVALID_PRICE;
    Quantity bid_size = 0;
    Quantity ask_size = 0;
    Timestamp timestamp = 0;
    
    // Derived values
    Price mid_price() const {
        if (best_bid != INVALID_PRICE && best_ask != INVALID_PRICE) {
            return (best_bid + best_ask) / 2;
        }
        return INVALID_PRICE;
    }
    
    Price spread() const {
        if (best_bid != INVALID_PRICE && best_ask != INVALID_PRICE) {
            return best_ask - best_bid;
        }
        return INVALID_PRICE;
    }
    
    nlohmann::json to_json() const;
};

// Multi-level depth snapshot
struct DepthSnapshot {
    std::string symbol;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
    Timestamp timestamp = 0;
    uint64_t sequence_number = 0;
    
    nlohmann::json to_json() const;
    
    // Serialize to binary format
    std::vector<uint8_t> to_binary() const;
    
    // Deserialize from binary format
    static DepthSnapshot from_binary(const std::vector<uint8_t>& data);
};

} // namespace lob

