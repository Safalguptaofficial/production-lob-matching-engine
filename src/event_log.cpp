#include "lob/event_log.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace lob {

EventLog::EventLog() = default;

EventLog::EventLog(const std::string& log_path, bool deterministic)
    : deterministic_(deterministic), log_path_(log_path) {
    if (deterministic_) {
        ensure_log_open();
    }
}

EventLog::~EventLog() {
    if (log_file_ && log_file_->is_open()) {
        log_file_->flush();
        log_file_->close();
    }
}

void EventLog::set_deterministic(bool enabled) {
    deterministic_ = enabled;
    if (deterministic_ && !log_path_.empty()) {
        ensure_log_open();
    }
}

void EventLog::set_log_path(const std::string& path) {
    log_path_ = path;
    if (deterministic_) {
        ensure_log_open();
    }
}

void EventLog::log_new_order(const NewOrderRequest& request) {
    if (!deterministic_) return;
    
    nlohmann::json j = request;
    write_log_entry("NEW_ORDER", j.dump());
}

void EventLog::log_cancel(const CancelRequest& request) {
    if (!deterministic_) return;
    
    nlohmann::json j = request;
    write_log_entry("CANCEL", j.dump());
}

void EventLog::log_replace(const ReplaceRequest& request) {
    if (!deterministic_) return;
    
    nlohmann::json j = request;
    write_log_entry("REPLACE", j.dump());
}

void EventLog::log_event(const OrderAcceptedEvent& event) {
    if (!deterministic_) return;
    
    nlohmann::json j;
    j["order_id"] = event.order_id;
    j["symbol"] = event.symbol;
    j["side"] = event.side;
    j["price"] = event.price;
    j["quantity"] = event.quantity;
    j["timestamp"] = event.timestamp;
    j["sequence_number"] = event.sequence_number;
    
    write_log_entry("ORDER_ACCEPTED", j.dump());
}

void EventLog::log_event(const OrderRejectedEvent& event) {
    if (!deterministic_) return;
    
    nlohmann::json j;
    j["order_id"] = event.order_id;
    j["symbol"] = event.symbol;
    j["reason"] = static_cast<int>(event.reason);
    j["message"] = event.message;
    j["timestamp"] = event.timestamp;
    j["sequence_number"] = event.sequence_number;
    
    write_log_entry("ORDER_REJECTED", j.dump());
}

void EventLog::log_event(const OrderCancelledEvent& event) {
    if (!deterministic_) return;
    
    nlohmann::json j;
    j["order_id"] = event.order_id;
    j["symbol"] = event.symbol;
    j["remaining_quantity"] = event.remaining_quantity;
    j["timestamp"] = event.timestamp;
    j["sequence_number"] = event.sequence_number;
    
    write_log_entry("ORDER_CANCELLED", j.dump());
}

void EventLog::log_event(const OrderReplacedEvent& event) {
    if (!deterministic_) return;
    
    nlohmann::json j;
    j["old_order_id"] = event.old_order_id;
    j["new_order_id"] = event.new_order_id;
    j["symbol"] = event.symbol;
    j["new_price"] = event.new_price;
    j["new_quantity"] = event.new_quantity;
    j["timestamp"] = event.timestamp;
    j["sequence_number"] = event.sequence_number;
    
    write_log_entry("ORDER_REPLACED", j.dump());
}

void EventLog::log_event(const TradeEvent& event) {
    if (!deterministic_) return;
    
    nlohmann::json j;
    j["trade_id"] = event.trade_id;
    j["symbol"] = event.symbol;
    j["price"] = event.price;
    j["quantity"] = event.quantity;
    j["aggressor_side"] = event.aggressor_side;
    j["aggressive_order_id"] = event.aggressive_order_id;
    j["passive_order_id"] = event.passive_order_id;
    j["aggressive_trader_id"] = event.aggressive_trader_id;
    j["passive_trader_id"] = event.passive_trader_id;
    j["timestamp"] = event.timestamp;
    j["sequence_number"] = event.sequence_number;
    
    write_log_entry("TRADE", j.dump());
}

void EventLog::flush() {
    if (log_file_ && log_file_->is_open()) {
        log_file_->flush();
    }
}

std::vector<EventLog::LogEntry> EventLog::load_log(const std::string& path) {
    std::vector<LogEntry> entries;
    std::ifstream file(path);
    
    if (!file.is_open()) {
        return entries;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        try {
            auto j = nlohmann::json::parse(line);
            
            LogEntry entry;
            entry.sequence_number = j["seq"];
            entry.timestamp = j["ts"];
            
            std::string type_str = j["type"];
            if (type_str == "NEW_ORDER") {
                entry.type = LogEntry::Type::NEW_ORDER;
            } else if (type_str == "CANCEL") {
                entry.type = LogEntry::Type::CANCEL;
            } else if (type_str == "REPLACE") {
                entry.type = LogEntry::Type::REPLACE;
            } else if (type_str == "ORDER_ACCEPTED") {
                entry.type = LogEntry::Type::ORDER_ACCEPTED;
            } else if (type_str == "ORDER_REJECTED") {
                entry.type = LogEntry::Type::ORDER_REJECTED;
            } else if (type_str == "ORDER_CANCELLED") {
                entry.type = LogEntry::Type::ORDER_CANCELLED;
            } else if (type_str == "ORDER_REPLACED") {
                entry.type = LogEntry::Type::ORDER_REPLACED;
            } else if (type_str == "TRADE") {
                entry.type = LogEntry::Type::TRADE;
            }
            
            entry.json_data = j["data"].dump();
            entries.push_back(entry);
        } catch (const std::exception&) {
            // Skip malformed lines
            continue;
        }
    }
    
    return entries;
}

void EventLog::ensure_log_open() {
    if (!log_file_ && !log_path_.empty()) {
        log_file_ = std::make_unique<std::ofstream>(log_path_, std::ios::app);
    }
}

void EventLog::write_log_entry(const std::string& type, const std::string& json_data) {
    ensure_log_open();
    
    if (!log_file_ || !log_file_->is_open()) {
        return;
    }
    
    nlohmann::json j;
    j["type"] = type;
    j["seq"] = ++sequence_number_;
    j["ts"] = std::chrono::duration_cast<std::chrono::nanoseconds>(
                  std::chrono::steady_clock::now().time_since_epoch())
                  .count();
    j["data"] = nlohmann::json::parse(json_data);
    
    *log_file_ << j.dump() << "\n";
}

} // namespace lob

