// Example: Connect to live exchange feed (WebSocket)
// This is a template - you'll need a WebSocket library (e.g., websocketpp, boost.beast)
#include <lob/matching_engine.hpp>
#include <iostream>
#include <thread>
#include <chrono>

/*
 * LIVE DATA SOURCES:
 * 
 * 1. Coinbase Pro WebSocket (Crypto - FREE)
 *    wss://ws-feed.pro.coinbase.com
 *    
 * 2. Binance WebSocket (Crypto - FREE)
 *    wss://stream.binance.com:9443/ws/<symbol>@trade
 *    
 * 3. IEX Cloud (Stocks - FREE tier)
 *    https://cloud.iex.io/
 *    
 * 4. Polygon.io (Stocks - FREE tier)
 *    wss://socket.polygon.io/
 *    
 * 5. Alpha Vantage (Stocks - FREE tier)
 *    https://www.alphavantage.co/
 */

// Example message handler for Coinbase Pro
struct CoinbaseMessage {
    std::string type;      // "match", "open", "done"
    std::string side;      // "buy" or "sell"
    double price;
    double size;
    std::string order_id;
    uint64_t timestamp;
};

class LiveFeedHandler {
public:
    LiveFeedHandler(lob::MatchingEngine& engine) : engine_(engine) {}
    
    void on_trade(const CoinbaseMessage& msg) {
        // Convert exchange message to LOB order
        lob::NewOrderRequest request{
            .order_id = std::hash<std::string>{}(msg.order_id),
            .trader_id = 1,
            .symbol = "BTC-USD",
            .side = msg.side == "buy" ? lob::Side::BUY : lob::Side::SELL,
            .order_type = lob::OrderType::MARKET,
            .price = static_cast<lob::Price>(msg.price * 100),
            .quantity = static_cast<lob::Quantity>(msg.size * 100),
            .time_in_force = lob::TimeInForce::IOC,
            .timestamp = msg.timestamp
        };
        
        auto response = engine_.handle(request);
        
        if (!response.trades.empty()) {
            std::cout << "Matched: " << response.trades.size() << " trades\n";
        }
    }
    
private:
    lob::MatchingEngine& engine_;
};

int main() {
    std::cout << "=== Live Market Data Feed Example ===\n\n";
    
    std::cout << "This is a template for connecting to live exchange feeds.\n";
    std::cout << "To implement:\n\n";
    
    std::cout << "1. Choose a data source:\n";
    std::cout << "   - Coinbase Pro (Crypto, FREE): wss://ws-feed.pro.coinbase.com\n";
    std::cout << "   - Binance (Crypto, FREE): wss://stream.binance.com:9443\n";
    std::cout << "   - Polygon.io (Stocks, FREE tier): wss://socket.polygon.io/\n\n";
    
    std::cout << "2. Add WebSocket library:\n";
    std::cout << "   Option A: websocketpp (header-only)\n";
    std::cout << "   Option B: boost.beast (requires Boost)\n";
    std::cout << "   Option C: uWebSockets (very fast)\n\n";
    
    std::cout << "3. Example Coinbase Pro subscription:\n";
    std::cout << R"(
{
    "type": "subscribe",
    "channels": [{
        "name": "matches",
        "product_ids": ["BTC-USD", "ETH-USD"]
    }]
}
)" << "\n";
    
    std::cout << "4. Parse JSON messages and feed to LOB engine\n\n";
    
    // Simulated example
    lob::MatchingEngine engine;
    engine.add_symbol({"BTC-USD", 1, 1, 1});
    
    std::cout << "Simulating 10 seconds of live data...\n";
    
    for (int i = 0; i < 10; ++i) {
        // Simulate incoming order
        lob::NewOrderRequest order{
            .order_id = static_cast<lob::OrderId>(i + 1),
            .trader_id = 1,
            .symbol = "BTC-USD",
            .side = (i % 2 == 0) ? lob::Side::BUY : lob::Side::SELL,
            .order_type = lob::OrderType::LIMIT,
            .price = 4000000 + (i * 100),  // $40,000.00
            .quantity = 100,
            .time_in_force = lob::TimeInForce::DAY
        };
        
        auto response = engine.handle(order);
        std::cout << "Order " << i+1 << ": " << response.trades.size() << " trades\n";
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "\n✅ Simulation complete!\n";
    std::cout << "\nTo implement real WebSocket connection:\n";
    std::cout << "1. Install websocketpp: brew install websocketpp\n";
    std::cout << "2. See examples/websocket_coinbase.cpp (to be created)\n";
    
    return 0;
}

