#pragma once

#include "events.hpp"
#include <vector>
#include <deque>
#include <string>

namespace lob {

// Maintains recent trade history for a symbol
class TradeTape {
public:
    explicit TradeTape(size_t max_history = 10000);
    
    // Add a trade to the tape
    void add_trade(const TradeEvent& trade);
    
    // Query recent trades
    std::vector<TradeEvent> get_recent_trades(size_t max_count) const;
    
    // Get all trades in the tape
    const std::deque<TradeEvent>& get_all_trades() const { return trades_; }
    
    // Clear the tape
    void clear();
    
    // Get trade count
    size_t size() const { return trades_.size(); }
    
    // Export trades to CSV format
    std::string to_csv() const;
    
private:
    std::deque<TradeEvent> trades_;
    size_t max_history_;
};

} // namespace lob

