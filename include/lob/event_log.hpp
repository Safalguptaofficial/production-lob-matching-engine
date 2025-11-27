#pragma once

#include "messages.hpp"
#include "events.hpp"
#include <string>
#include <fstream>
#include <vector>
#include <memory>

namespace lob {

// Event log for deterministic replay
class EventLog {
public:
    EventLog();
    explicit EventLog(const std::string& log_path, bool deterministic = false);
    ~EventLog();
    
    // Enable/disable deterministic mode
    void set_deterministic(bool enabled);
    bool is_deterministic() const { return deterministic_; }
    
    // Set output path
    void set_log_path(const std::string& path);
    
    // Log incoming messages
    void log_new_order(const NewOrderRequest& request);
    void log_cancel(const CancelRequest& request);
    void log_replace(const ReplaceRequest& request);
    
    // Log resulting events
    void log_event(const OrderAcceptedEvent& event);
    void log_event(const OrderRejectedEvent& event);
    void log_event(const OrderCancelledEvent& event);
    void log_event(const OrderReplacedEvent& event);
    void log_event(const TradeEvent& event);
    
    // Flush buffered writes
    void flush();
    
    // Replay interface
    struct LogEntry {
        enum class Type {
            NEW_ORDER, CANCEL, REPLACE,
            ORDER_ACCEPTED, ORDER_REJECTED, ORDER_CANCELLED, ORDER_REPLACED, TRADE
        };
        Type type;
        std::string json_data;
        uint64_t sequence_number;
        Timestamp timestamp;
    };
    
    // Load log entries for replay
    std::vector<LogEntry> load_log(const std::string& path);
    
private:
    bool deterministic_ = false;
    std::string log_path_;
    std::unique_ptr<std::ofstream> log_file_;
    uint64_t sequence_number_ = 0;
    
    void ensure_log_open();
    void write_log_entry(const std::string& type, const std::string& json_data);
};

} // namespace lob

