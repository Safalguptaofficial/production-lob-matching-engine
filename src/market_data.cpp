#include "lob/market_data.hpp"

namespace lob {

// ============================================================================
// TopOfBook Implementation
// ============================================================================

nlohmann::json TopOfBook::to_json() const {
    nlohmann::json j;
    j["symbol"] = symbol;
    j["timestamp"] = timestamp;
    j["best_bid"] = best_bid;
    j["best_ask"] = best_ask;
    j["bid_size"] = bid_size;
    j["ask_size"] = ask_size;
    j["mid_price"] = mid_price();
    j["spread"] = spread();
    return j;
}

// ============================================================================
// DepthSnapshot Implementation
// ============================================================================

nlohmann::json DepthSnapshot::to_json() const {
    nlohmann::json j;
    j["symbol"] = symbol;
    j["timestamp"] = timestamp;
    j["sequence_number"] = sequence_number;
    
    j["bids"] = nlohmann::json::array();
    for (const auto& level : bids) {
        j["bids"].push_back({{"price", level.price},
                             {"quantity", level.quantity},
                             {"order_count", level.order_count}});
    }
    
    j["asks"] = nlohmann::json::array();
    for (const auto& level : asks) {
        j["asks"].push_back({{"price", level.price},
                             {"quantity", level.quantity},
                             {"order_count", level.order_count}});
    }
    
    return j;
}

std::vector<uint8_t> DepthSnapshot::to_binary() const {
    // Binary format is implemented in binary_serializer.cpp
    // For now, return empty vector as placeholder
    std::vector<uint8_t> buffer;
    return buffer;
}

DepthSnapshot DepthSnapshot::from_binary(const std::vector<uint8_t>& /*data*/) {
    // Binary deserialization is implemented in binary_serializer.cpp
    // For now, return empty snapshot as placeholder
    DepthSnapshot snapshot;
    return snapshot;
}

} // namespace lob

