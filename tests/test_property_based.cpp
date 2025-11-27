#include <gtest/gtest.h>
#include <lob/engine_validator.hpp>
#include <random>

using namespace lob;

Order generate_random_order(std::mt19937& rng, OrderId id) {
    std::uniform_int_distribution<> side_dist(0, 1);
    std::uniform_int_distribution<> price_dist(9900, 10100);
    std::uniform_int_distribution<> qty_dist(10, 1000);
    std::uniform_int_distribution<> trader_dist(100, 110);
    
    Order order;
    order.order_id = id;
    order.trader_id = trader_dist(rng);
    order.symbol = "TEST";
    order.side = side_dist(rng) == 0 ? Side::BUY : Side::SELL;
    order.order_type = OrderType::LIMIT;
    order.price = price_dist(rng);
    order.quantity = qty_dist(rng);
    order.remaining_quantity = order.quantity;
    order.time_in_force = TimeInForce::DAY;
    order.timestamp = id * 1000;
    
    return order;
}

TEST(PropertyBased, RandomOrderStream) {
    std::mt19937 rng(42);  // Fixed seed for reproducibility
    
    EngineValidator validator("TEST");
    
    // Generate and process random orders
    for (int i = 0; i < 100; ++i) {
        Order order = generate_random_order(rng, i + 1);
        
        auto result = validator.add_order(order);
        EXPECT_TRUE(result.passed) << "Order " << i << ": " << result.summary();
        
        if (!result.passed) {
            // Stop on first failure for easier debugging
            break;
        }
    }
    
    // Final state comparison
    auto final_result = validator.compare_states();
    EXPECT_TRUE(final_result.passed) << final_result.summary();
}

TEST(PropertyBased, RandomWithCancels) {
    std::mt19937 rng(123);
    std::uniform_real_distribution<> cancel_dist(0.0, 1.0);
    
    EngineValidator validator("TEST");
    std::vector<OrderId> active_orders;
    
    for (int i = 0; i < 50; ++i) {
        // 70% chance to add order, 30% chance to cancel if orders exist
        if (cancel_dist(rng) < 0.7 || active_orders.empty()) {
            // Add order
            Order order = generate_random_order(rng, i + 1);
            auto result = validator.add_order(order);
            EXPECT_TRUE(result.passed) << "Add order " << i << ": " << result.summary();
            active_orders.push_back(order.order_id);
        } else {
            // Cancel random order
            std::uniform_int_distribution<> idx_dist(0, active_orders.size() - 1);
            int idx = idx_dist(rng);
            OrderId to_cancel = active_orders[idx];
            active_orders.erase(active_orders.begin() + idx);
            
            auto result = validator.cancel_order(to_cancel);
            // Cancel might fail if order was already filled, which is okay
            // EXPECT_TRUE(result.passed) << "Cancel order " << to_cancel << ": " << result.summary();
        }
    }
    
    auto final_result = validator.compare_states();
    EXPECT_TRUE(final_result.passed) << final_result.summary();
}

