// Live Market Data Feed Simulator
// Simulates a real-time market data feed with WebSocket-like behavior
#include <lob/matching_engine.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>

class MarketDataFeedSimulator {
public:
    MarketDataFeedSimulator(lob::MatchingEngine& engine, 
                           const std::vector<std::string>& symbols)
        : engine_(engine), symbols_(symbols), running_(false) {
        
        // Initialize mid prices
        for (const auto& symbol : symbols) {
            mid_prices_[symbol] = 150.0 + (rand() % 100);
        }
    }
    
    void start() {
        running_ = true;
        feed_thread_ = std::thread(&MarketDataFeedSimulator::feed_loop, this);
    }
    
    void stop() {
        running_ = false;
        if (feed_thread_.joinable()) {
            feed_thread_.join();
        }
    }
    
    ~MarketDataFeedSimulator() {
        stop();
    }
    
private:
    void feed_loop() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> action_dist(0.0, 1.0);
        std::uniform_real_distribution<> price_dist(-2.0, 2.0);
        std::uniform_int_distribution<> qty_dist(10, 500);
        std::uniform_int_distribution<> symbol_dist(0, symbols_.size() - 1);
        
        uint64_t order_id = 1;
        
        while (running_) {
            // Pick random symbol
            std::string symbol = symbols_[symbol_dist(gen)];
            double mid = mid_prices_[symbol];
            
            // 70% new orders, 20% market orders, 10% updates
            double action = action_dist(gen);
            
            if (action < 0.7) {
                // Post limit order
                bool is_buy = action_dist(gen) < 0.5;
                double offset = std::abs(price_dist(gen));
                double price = is_buy ? (mid - offset) : (mid + offset);
                
                lob::NewOrderRequest order{
                    .order_id = order_id++,
                    .trader_id = static_cast<uint64_t>(qty_dist(gen) % 10 + 1),
                    .symbol = symbol,
                    .side = is_buy ? lob::Side::BUY : lob::Side::SELL,
                    .order_type = lob::OrderType::LIMIT,
                    .price = static_cast<lob::Price>(price * 100),
                    .quantity = static_cast<uint64_t>(qty_dist(gen)),
                    .time_in_force = lob::TimeInForce::DAY
                };
                
                auto response = engine_.handle(order);
                
                if (!response.trades.empty()) {
                    for (const auto& trade : response.trades) {
                        std::cout << "[LIVE] " << symbol << " TRADE: "
                                  << trade.quantity << " @ $"
                                  << trade.price / 100.0 << " | "
                                  << lob::side_to_string(trade.aggressor_side) << "\n";
                    }
                }
                
            } else if (action < 0.9) {
                // Send aggressive market order
                bool is_buy = action_dist(gen) < 0.5;
                
                lob::NewOrderRequest order{
                    .order_id = order_id++,
                    .trader_id = static_cast<uint64_t>(qty_dist(gen) % 10 + 1),
                    .symbol = symbol,
                    .side = is_buy ? lob::Side::BUY : lob::Side::SELL,
                    .order_type = lob::OrderType::MARKET,
                    .price = 0,
                    .quantity = static_cast<uint64_t>(qty_dist(gen) / 2),
                    .time_in_force = lob::TimeInForce::DAY
                };
                
                auto response = engine_.handle(order);
                
                if (!response.trades.empty()) {
                    std::cout << "[LIVE] " << symbol << " MARKET "
                              << lob::side_to_string(order.side) << ": "
                              << response.trades.size() << " fills, "
                              << "avg price $" 
                              << calculate_avg_price(response.trades) << "\n";
                }
            }
            
            // Drift mid price
            if (order_id % 50 == 0) {
                mid_prices_[symbol] += price_dist(gen) * 0.1;
            }
            
            // Print book state periodically
            if (order_id % 100 == 0) {
                print_market_state();
            }
            
            // Sleep to simulate realistic timing (1-10ms between orders)
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1 + (rand() % 10))
            );
        }
    }
    
    double calculate_avg_price(const std::vector<lob::TradeEvent>& trades) {
        if (trades.empty()) return 0.0;
        
        uint64_t total_value = 0;
        uint64_t total_qty = 0;
        
        for (const auto& trade : trades) {
            total_value += trade.price * trade.quantity;
            total_qty += trade.quantity;
        }
        
        return (total_value / 100.0) / total_qty;
    }
    
    void print_market_state() {
        std::cout << "\n=== Market State ===\n";
        for (const auto& symbol : symbols_) {
            auto tob = engine_.get_top_of_book(symbol);
            if (tob.best_bid != lob::INVALID_PRICE && 
                tob.best_ask != lob::INVALID_PRICE) {
                std::cout << symbol << ": "
                          << "$" << tob.best_bid / 100.0 << " x $"
                          << tob.best_ask / 100.0 
                          << " (spread: $" << tob.spread() / 100.0 << ")\n";
            }
        }
        std::cout << std::endl;
    }
    
    lob::MatchingEngine& engine_;
    std::vector<std::string> symbols_;
    std::unordered_map<std::string, double> mid_prices_;
    std::thread feed_thread_;
    std::atomic<bool> running_;
};


int main(int argc, char** argv) {
    std::cout << "=== Live Market Data Feed Simulator ===\n\n";
    
    int duration_seconds = 30;  // Default 30 seconds
    if (argc > 1) {
        duration_seconds = std::stoi(argv[1]);
    }
    
    std::cout << "Running for " << duration_seconds << " seconds\n";
    std::cout << "Press Ctrl+C to stop early\n\n";
    
    // Create engine
    lob::MatchingEngine engine;
    
    // Add symbols
    std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOGL"};
    for (const auto& symbol : symbols) {
        lob::SymbolConfig config{symbol, 1, 1, 1};
        engine.add_symbol(config);
        std::cout << "Streaming " << symbol << "\n";
    }
    
    std::cout << "\n=== Starting Live Feed ===\n\n";
    
    // Start simulator
    MarketDataFeedSimulator simulator(engine, symbols);
    simulator.start();
    
    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    
    // Stop simulator
    simulator.stop();
    
    std::cout << "\n=== Feed Stopped ===\n\n";
    
    // Print final statistics
    std::cout << "=== Final Statistics ===\n";
    std::cout << engine.get_telemetry_json().dump(2) << "\n";
    
    // Print final book states
    std::cout << "\n=== Final Book States ===\n";
    for (const auto& symbol : symbols) {
        auto depth = engine.get_depth_snapshot(symbol, 5);
        std::cout << "\n" << symbol << " (Top 5 levels):\n";
        
        std::cout << "  Bids:\n";
        for (const auto& level : depth.bids) {
            std::cout << "    $" << level.price / 100.0 << " : " 
                      << level.quantity << "\n";
        }
        
        std::cout << "  Asks:\n";
        for (const auto& level : depth.asks) {
            std::cout << "    $" << level.price / 100.0 << " : " 
                      << level.quantity << "\n";
        }
    }
    
    return 0;
}

