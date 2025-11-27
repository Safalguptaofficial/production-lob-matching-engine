#include "lob/trade_tape.hpp"
#include <sstream>

namespace lob {

TradeTape::TradeTape(size_t max_history) : max_history_(max_history) {}

void TradeTape::add_trade(const TradeEvent& trade) {
    trades_.push_back(trade);
    
    // Maintain max history size
    while (trades_.size() > max_history_) {
        trades_.pop_front();
    }
}

std::vector<TradeEvent> TradeTape::get_recent_trades(size_t max_count) const {
    std::vector<TradeEvent> result;
    
    size_t count = std::min(max_count, trades_.size());
    auto it = trades_.end();
    std::advance(it, -static_cast<long>(count));
    
    result.assign(it, trades_.end());
    return result;
}

void TradeTape::clear() {
    trades_.clear();
}

std::string TradeTape::to_csv() const {
    std::ostringstream oss;
    
    // Header
    oss << "trade_id,symbol,timestamp,price,quantity,side,"
        << "aggressive_order_id,passive_order_id,aggressive_trader_id,passive_trader_id\n";
    
    // Data rows
    for (const auto& trade : trades_) {
        oss << trade.trade_id << "," << trade.symbol << "," << trade.timestamp << ","
            << trade.price << "," << trade.quantity << ","
            << side_to_string(trade.aggressor_side) << "," << trade.aggressive_order_id << ","
            << trade.passive_order_id << "," << trade.aggressive_trader_id << ","
            << trade.passive_trader_id << "\n";
    }
    
    return oss.str();
}

} // namespace lob

