#pragma once

#include "order_book.hpp"
#include "reference_order_book.hpp"
#include "events.hpp"
#include <string>
#include <vector>

namespace lob {

// Validation result
struct ValidationResult {
    bool passed = true;
    std::vector<std::string> mismatches;
    
    void add_mismatch(const std::string& msg) {
        passed = false;
        mismatches.push_back(msg);
    }
    
    std::string summary() const;
};

// Dual-engine validator that runs both optimized and reference implementations
class EngineValidator {
public:
    EngineValidator(const std::string& symbol, STPPolicy stp_policy = STPPolicy::CANCEL_INCOMING);
    
    // Run order on both engines and compare
    ValidationResult add_order(const Order& order);
    ValidationResult cancel_order(OrderId order_id);
    ValidationResult replace_order(OrderId order_id, Price new_price, Quantity new_quantity);
    
    // Compare final states
    ValidationResult compare_states() const;
    
    // Access engines
    const OrderBook& optimized_book() const { return *optimized_; }
    const ReferenceOrderBook& reference_book() const { return *reference_; }
    
private:
    std::unique_ptr<OrderBook> optimized_;
    std::unique_ptr<ReferenceOrderBook> reference_;
    
    // Compare trade events
    bool compare_trades(const std::vector<TradeEvent>& optimized_trades,
                       const std::vector<TradeEvent>& reference_trades,
                       ValidationResult& result) const;
    
    // Compare book states
    bool compare_top_of_book(ValidationResult& result) const;
    bool compare_depth(ValidationResult& result) const;
};

} // namespace lob

