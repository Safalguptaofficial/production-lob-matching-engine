#include "lob/engine_validator.hpp"
#include <sstream>

namespace lob {

std::string ValidationResult::summary() const {
    if (passed) {
        return "✓ PASSED";
    }
    
    std::ostringstream oss;
    oss << "✗ FAILED:\n";
    for (const auto& mismatch : mismatches) {
        oss << "  - " << mismatch << "\n";
    }
    
    return oss.str();
}

EngineValidator::EngineValidator(const std::string& symbol, STPPolicy stp_policy) {
    optimized_ = std::make_unique<OrderBook>(symbol, stp_policy);
    reference_ = std::make_unique<ReferenceOrderBook>(symbol, stp_policy);
}

ValidationResult EngineValidator::add_order(const Order& order) {
    ValidationResult result;
    
    // Execute on both engines
    auto optimized_trades = optimized_->add_order(order);
    auto reference_trades = reference_->add_order(order);
    
    // Compare trades
    if (!compare_trades(optimized_trades, reference_trades, result)) {
        return result;
    }
    
    // Compare book states
    compare_top_of_book(result);
    
    return result;
}

ValidationResult EngineValidator::cancel_order(OrderId order_id) {
    ValidationResult result;
    
    bool optimized_cancelled = optimized_->cancel_order(order_id);
    bool reference_cancelled = reference_->cancel_order(order_id);
    
    if (optimized_cancelled != reference_cancelled) {
        result.add_mismatch("Cancel result mismatch: optimized=" +
                           std::to_string(optimized_cancelled) +
                           ", reference=" + std::to_string(reference_cancelled));
    }
    
    compare_top_of_book(result);
    
    return result;
}

ValidationResult EngineValidator::replace_order(OrderId order_id, Price new_price,
                                               Quantity new_quantity) {
    ValidationResult result;
    
    auto optimized_trades = optimized_->replace_order(order_id, new_price, new_quantity);
    auto reference_trades = reference_->replace_order(order_id, new_price, new_quantity);
    
    compare_trades(optimized_trades, reference_trades, result);
    compare_top_of_book(result);
    
    return result;
}

ValidationResult EngineValidator::compare_states() const {
    ValidationResult result;
    
    compare_top_of_book(result);
    compare_depth(result);
    
    return result;
}

// ============================================================================
// Private Methods
// ============================================================================

bool EngineValidator::compare_trades(const std::vector<TradeEvent>& optimized_trades,
                                    const std::vector<TradeEvent>& reference_trades,
                                    ValidationResult& result) const {
    if (optimized_trades.size() != reference_trades.size()) {
        result.add_mismatch("Trade count mismatch: optimized=" +
                           std::to_string(optimized_trades.size()) + ", reference=" +
                           std::to_string(reference_trades.size()));
        return false;
    }
    
    for (size_t i = 0; i < optimized_trades.size(); ++i) {
        const auto& opt_trade = optimized_trades[i];
        const auto& ref_trade = reference_trades[i];
        
        if (opt_trade.price != ref_trade.price) {
            result.add_mismatch("Trade " + std::to_string(i) + " price mismatch: optimized=" +
                               std::to_string(opt_trade.price) +
                               ", reference=" + std::to_string(ref_trade.price));
        }
        
        if (opt_trade.quantity != ref_trade.quantity) {
            result.add_mismatch("Trade " + std::to_string(i) +
                               " quantity mismatch: optimized=" +
                               std::to_string(opt_trade.quantity) + ", reference=" +
                               std::to_string(ref_trade.quantity));
        }
        
        if (opt_trade.aggressive_order_id != ref_trade.aggressive_order_id) {
            result.add_mismatch("Trade " + std::to_string(i) +
                               " aggressive_order_id mismatch");
        }
        
        if (opt_trade.passive_order_id != ref_trade.passive_order_id) {
            result.add_mismatch("Trade " + std::to_string(i) + " passive_order_id mismatch");
        }
    }
    
    return result.passed;
}

bool EngineValidator::compare_top_of_book(ValidationResult& result) const {
    auto opt_best_bid = optimized_->get_best_bid();
    auto ref_best_bid = reference_->get_best_bid();
    
    if (opt_best_bid != ref_best_bid) {
        std::string opt_str = opt_best_bid ? std::to_string(*opt_best_bid) : "NONE";
        std::string ref_str = ref_best_bid ? std::to_string(*ref_best_bid) : "NONE";
        result.add_mismatch("Best bid mismatch: optimized=" + opt_str + ", reference=" + ref_str);
    }
    
    auto opt_best_ask = optimized_->get_best_ask();
    auto ref_best_ask = reference_->get_best_ask();
    
    if (opt_best_ask != ref_best_ask) {
        std::string opt_str = opt_best_ask ? std::to_string(*opt_best_ask) : "NONE";
        std::string ref_str = ref_best_ask ? std::to_string(*ref_best_ask) : "NONE";
        result.add_mismatch("Best ask mismatch: optimized=" + opt_str + ", reference=" + ref_str);
    }
    
    return result.passed;
}

bool EngineValidator::compare_depth(ValidationResult& result) const {
    auto opt_depth = optimized_->get_depth_snapshot(10, 0);
    auto ref_depth = reference_->get_depth_snapshot(10, 0);
    
    if (opt_depth.bids.size() != ref_depth.bids.size()) {
        result.add_mismatch("Bid level count mismatch: optimized=" +
                           std::to_string(opt_depth.bids.size()) +
                           ", reference=" + std::to_string(ref_depth.bids.size()));
    }
    
    if (opt_depth.asks.size() != ref_depth.asks.size()) {
        result.add_mismatch("Ask level count mismatch: optimized=" +
                           std::to_string(opt_depth.asks.size()) +
                           ", reference=" + std::to_string(ref_depth.asks.size()));
    }
    
    // Compare each bid level
    size_t min_bids = std::min(opt_depth.bids.size(), ref_depth.bids.size());
    for (size_t i = 0; i < min_bids; ++i) {
        if (opt_depth.bids[i].price != ref_depth.bids[i].price ||
            opt_depth.bids[i].quantity != ref_depth.bids[i].quantity) {
            result.add_mismatch("Bid level " + std::to_string(i) + " mismatch");
        }
    }
    
    // Compare each ask level
    size_t min_asks = std::min(opt_depth.asks.size(), ref_depth.asks.size());
    for (size_t i = 0; i < min_asks; ++i) {
        if (opt_depth.asks[i].price != ref_depth.asks[i].price ||
            opt_depth.asks[i].quantity != ref_depth.asks[i].quantity) {
            result.add_mismatch("Ask level " + std::to_string(i) + " mismatch");
        }
    }
    
    return result.passed;
}

} // namespace lob

