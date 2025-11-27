#include <gtest/gtest.h>
#include <lob/engine_validator.hpp>

using namespace lob;

TEST(EngineComparison, SimpleCross) {
    EngineValidator validator("TEST");
    
    // Add sell order
    Order sell_order;
    sell_order.order_id = 1;
    sell_order.trader_id = 100;
    sell_order.symbol = "TEST";
    sell_order.side = Side::SELL;
    sell_order.order_type = OrderType::LIMIT;
    sell_order.price = 10000;
    sell_order.quantity = 100;
    sell_order.remaining_quantity = 100;
    sell_order.time_in_force = TimeInForce::DAY;
    sell_order.timestamp = 1000;
    
    auto result1 = validator.add_order(sell_order);
    EXPECT_TRUE(result1.passed) << result1.summary();
    
    // Add buy order that crosses
    Order buy_order;
    buy_order.order_id = 2;
    buy_order.trader_id = 101;
    buy_order.symbol = "TEST";
    buy_order.side = Side::BUY;
    buy_order.order_type = OrderType::LIMIT;
    buy_order.price = 10000;
    buy_order.quantity = 100;
    buy_order.remaining_quantity = 100;
    buy_order.time_in_force = TimeInForce::DAY;
    buy_order.timestamp = 2000;
    
    auto result2 = validator.add_order(buy_order);
    EXPECT_TRUE(result2.passed) << result2.summary();
    
    auto final_result = validator.compare_states();
    EXPECT_TRUE(final_result.passed) << final_result.summary();
}

TEST(EngineComparison, MultipleOrders) {
    EngineValidator validator("TEST");
    
    // Add multiple orders
    for (int i = 0; i < 10; ++i) {
        Order order;
        order.order_id = i + 1;
        order.trader_id = 100 + i;
        order.symbol = "TEST";
        order.side = (i % 2 == 0) ? Side::BUY : Side::SELL;
        order.order_type = OrderType::LIMIT;
        order.price = 10000 + (i % 2 == 0 ? -i : i);
        order.quantity = 100;
        order.remaining_quantity = 100;
        order.time_in_force = TimeInForce::DAY;
        order.timestamp = 1000 + i;
        
        auto result = validator.add_order(order);
        EXPECT_TRUE(result.passed) << "Order " << i << ": " << result.summary();
    }
    
    auto final_result = validator.compare_states();
    EXPECT_TRUE(final_result.passed) << final_result.summary();
}

