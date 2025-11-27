#include <gtest/gtest.h>
#include <lob/order_book.hpp>

using namespace lob;

TEST(OrderBookBasic, EmptyBook) {
    OrderBook book("TEST");
    
    EXPECT_FALSE(book.get_best_bid().has_value());
    EXPECT_FALSE(book.get_best_ask().has_value());
    EXPECT_EQ(book.active_order_count(), 0);
}

TEST(OrderBookBasic, SingleBuyOrder) {
    OrderBook book("TEST");
    
    Order order;
    order.order_id = 1;
    order.trader_id = 100;
    order.symbol = "TEST";
    order.side = Side::BUY;
    order.order_type = OrderType::LIMIT;
    order.price = 10000;
    order.quantity = 100;
    order.remaining_quantity = 100;
    order.time_in_force = TimeInForce::DAY;
    order.timestamp = 1000;
    
    auto trades = book.add_order(order);
    
    EXPECT_EQ(trades.size(), 0);  // No trades (no opposite side)
    EXPECT_TRUE(book.get_best_bid().has_value());
    EXPECT_EQ(book.get_best_bid().value(), 10000);
    EXPECT_FALSE(book.get_best_ask().has_value());
    EXPECT_EQ(book.active_order_count(), 1);
}

TEST(OrderBookBasic, SimpleCross) {
    OrderBook book("TEST");
    
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
    
    book.add_order(sell_order);
    
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
    
    auto trades = book.add_order(buy_order);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 10000);
    EXPECT_EQ(trades[0].quantity, 100);
    EXPECT_EQ(trades[0].aggressive_order_id, 2);
    EXPECT_EQ(trades[0].passive_order_id, 1);
    EXPECT_EQ(trades[0].aggressor_side, Side::BUY);
    
    // Both orders fully filled
    EXPECT_FALSE(book.get_best_bid().has_value());
    EXPECT_FALSE(book.get_best_ask().has_value());
    EXPECT_EQ(book.active_order_count(), 0);
}

TEST(OrderBookBasic, MarketOrder) {
    OrderBook book("TEST");
    
    // Add limit sell order
    Order limit_sell;
    limit_sell.order_id = 1;
    limit_sell.trader_id = 100;
    limit_sell.symbol = "TEST";
    limit_sell.side = Side::SELL;
    limit_sell.order_type = OrderType::LIMIT;
    limit_sell.price = 10000;
    limit_sell.quantity = 100;
    limit_sell.remaining_quantity = 100;
    limit_sell.time_in_force = TimeInForce::DAY;
    limit_sell.timestamp = 1000;
    
    book.add_order(limit_sell);
    
    // Add market buy order
    Order market_buy;
    market_buy.order_id = 2;
    market_buy.trader_id = 101;
    market_buy.symbol = "TEST";
    market_buy.side = Side::BUY;
    market_buy.order_type = OrderType::MARKET;
    market_buy.price = 0;  // Market order
    market_buy.quantity = 50;
    market_buy.remaining_quantity = 50;
    market_buy.time_in_force = TimeInForce::DAY;
    market_buy.timestamp = 2000;
    
    auto trades = book.add_order(market_buy);
    
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_EQ(trades[0].price, 10000);  // Trades at limit order price
    
    // Limit order partially filled
    EXPECT_TRUE(book.get_best_ask().has_value());
    EXPECT_EQ(book.get_best_ask().value(), 10000);
}

TEST(OrderBookBasic, CancelOrder) {
    OrderBook book("TEST");
    
    Order order;
    order.order_id = 1;
    order.trader_id = 100;
    order.symbol = "TEST";
    order.side = Side::BUY;
    order.order_type = OrderType::LIMIT;
    order.price = 10000;
    order.quantity = 100;
    order.remaining_quantity = 100;
    order.time_in_force = TimeInForce::DAY;
    order.timestamp = 1000;
    
    book.add_order(order);
    EXPECT_EQ(book.active_order_count(), 1);
    
    bool cancelled = book.cancel_order(1);
    EXPECT_TRUE(cancelled);
    EXPECT_EQ(book.active_order_count(), 0);
    EXPECT_FALSE(book.get_best_bid().has_value());
    
    // Cancel non-existent order
    bool cancelled2 = book.cancel_order(999);
    EXPECT_FALSE(cancelled2);
}

