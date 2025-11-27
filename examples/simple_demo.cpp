// Simple demo of the LOB matching engine
#include <lob/matching_engine.hpp>
#include <iostream>
#include <iomanip>

void print_separator() {
    std::cout << std::string(80, '=') << "\n";
}

void print_top_of_book(const lob::TopOfBook& tob) {
    std::cout << "Top of Book (" << tob.symbol << "):\n";
    std::cout << "  Best Bid: " << std::fixed << std::setprecision(2) 
              << tob.best_bid / 100.0 << " (" << tob.bid_size << " shares)\n";
    std::cout << "  Best Ask: " << tob.best_ask / 100.0 << " (" << tob.ask_size << " shares)\n";
    std::cout << "  Spread:   " << tob.spread() / 100.0 << "\n";
    std::cout << "  Mid:      " << tob.mid_price() / 100.0 << "\n";
}

void print_trade(const lob::TradeEvent& trade) {
    std::cout << "TRADE: " << lob::side_to_string(trade.aggressor_side) 
              << " " << trade.quantity << " @ " << std::fixed << std::setprecision(2)
              << trade.price / 100.0 << " (IDs: " << trade.aggressive_order_id 
              << " x " << trade.passive_order_id << ")\n";
}

int main() {
    std::cout << "HFT Limit Order Book - Simple Demo\n";
    print_separator();
    
    // Create matching engine
    lob::MatchingEngine engine;
    
    // Register a symbol (AAPL)
    lob::SymbolConfig config{"AAPL", 1, 1, 1};  // tick=1 cent, lot=1, min_qty=1
    engine.add_symbol(config);
    
    std::cout << "\n1. Adding initial buy orders...\n";
    
    // Add buy orders at different price levels
    lob::NewOrderRequest buy1{
        .order_id = 1,
        .trader_id = 100,
        .symbol = "AAPL",
        .side = lob::Side::BUY,
        .order_type = lob::OrderType::LIMIT,
        .price = 15000,  // $150.00
        .quantity = 100,
        .time_in_force = lob::TimeInForce::DAY,
        .timestamp = 1000
    };
    
    engine.handle(buy1);
    
    lob::NewOrderRequest buy2{
        .order_id = 2,
        .trader_id = 100,
        .symbol = "AAPL",
        .side = lob::Side::BUY,
        .order_type = lob::OrderType::LIMIT,
        .price = 14999,  // $149.99
        .quantity = 200,
        .time_in_force = lob::TimeInForce::DAY,
        .timestamp = 2000
    };
    
    engine.handle(buy2);
    
    std::cout << "\n2. Adding initial sell orders...\n";
    
    lob::NewOrderRequest sell1{
        .order_id = 3,
        .trader_id = 101,
        .symbol = "AAPL",
        .side = lob::Side::SELL,
        .order_type = lob::OrderType::LIMIT,
        .price = 15001,  // $150.01
        .quantity = 150,
        .time_in_force = lob::TimeInForce::DAY,
        .timestamp = 3000
    };
    
    engine.handle(sell1);
    
    std::cout << "\n";
    print_top_of_book(engine.get_top_of_book("AAPL"));
    
    print_separator();
    std::cout << "\n3. Sending aggressive buy order that crosses the spread...\n\n";
    
    lob::NewOrderRequest aggressive_buy{
        .order_id = 4,
        .trader_id = 102,
        .symbol = "AAPL",
        .side = lob::Side::BUY,
        .order_type = lob::OrderType::LIMIT,
        .price = 15001,  // $150.01 - crosses spread
        .quantity = 100,
        .time_in_force = lob::TimeInForce::DAY,
        .timestamp = 4000
    };
    
    auto response = engine.handle(aggressive_buy);
    
    for (const auto& trade : response.trades) {
        print_trade(trade);
    }
    
    std::cout << "\n";
    print_top_of_book(engine.get_top_of_book("AAPL"));
    
    print_separator();
    std::cout << "\n4. Sending market sell order...\n\n";
    
    lob::NewOrderRequest market_sell{
        .order_id = 5,
        .trader_id = 103,
        .symbol = "AAPL",
        .side = lob::Side::SELL,
        .order_type = lob::OrderType::MARKET,
        .price = 0,  // Market order - price ignored
        .quantity = 150,
        .time_in_force = lob::TimeInForce::DAY,
        .timestamp = 5000
    };
    
    response = engine.handle(market_sell);
    
    for (const auto& trade : response.trades) {
        print_trade(trade);
    }
    
    std::cout << "\n";
    print_top_of_book(engine.get_top_of_book("AAPL"));
    
    print_separator();
    std::cout << "\nFinal Statistics:\n";
    std::cout << engine.get_telemetry_json().dump(2) << "\n";
    
    print_separator();
    std::cout << "\nDemo complete!\n";
    
    return 0;
}

