#pragma once

#include "events.hpp"
#include <memory>

namespace lob {

// Interface for receiving engine events
class IMatchingEngineListener {
public:
    virtual ~IMatchingEngineListener() = default;
    
    // Order lifecycle events
    virtual void on_order_accepted(const OrderAcceptedEvent& event) = 0;
    virtual void on_order_rejected(const OrderRejectedEvent& event) = 0;
    virtual void on_order_cancelled(const OrderCancelledEvent& event) = 0;
    virtual void on_order_replaced(const OrderReplacedEvent& event) = 0;
    
    // Trade events
    virtual void on_trade(const TradeEvent& event) = 0;
    
    // Book update events (optional, for market data feeds)
    virtual void on_book_update(const BookUpdateEvent& event) = 0;
};

// Base class with no-op implementations
class MatchingEngineListenerBase : public IMatchingEngineListener {
public:
    void on_order_accepted(const OrderAcceptedEvent&) override {}
    void on_order_rejected(const OrderRejectedEvent&) override {}
    void on_order_cancelled(const OrderCancelledEvent&) override {}
    void on_order_replaced(const OrderReplacedEvent&) override {}
    void on_trade(const TradeEvent&) override {}
    void on_book_update(const BookUpdateEvent&) override {}
};

} // namespace lob

