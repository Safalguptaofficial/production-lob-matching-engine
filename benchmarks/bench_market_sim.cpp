// Market simulator benchmark with realistic order patterns
#include <lob/matching_engine.hpp>
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace lob;

int main() {
    std::cout << "=== LOB Market Simulator Benchmark ===\n\n";
    
    const int NUM_ORDERS = 50000;
    const double CANCEL_RATIO = 0.4;  // 40% of orders are cancels
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<> action_dist(0.0, 1.0);
    std::uniform_int_distribution<> side_dist(0, 1);
    std::uniform_int_distribution<> spread_dist(1, 5);
    std::uniform_int_distribution<> qty_dist(100, 1000);
    std::uniform_int_distribution<> trader_dist(100, 120);
    
    MatchingEngine engine;
    
    // Add multiple symbols
    std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOGL"};
    for (const auto& symbol : symbols) {
        SymbolConfig config{symbol, 1, 1, 1};
        engine.add_symbol(config);
    }
    
    std::cout << "Simulating " << NUM_ORDERS << " market actions...\n";
    std::cout << "Symbols: " << symbols.size() << "\n";
    std::cout << "Cancel ratio: " << (CANCEL_RATIO * 100) << "%\n\n";
    
    Price mid_price = 10000;  // $100.00
    std::vector<OrderId> active_orders;
    OrderId next_order_id = 1;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_ORDERS; ++i) {
        // Decide: new order or cancel
        if (action_dist(rng) < CANCEL_RATIO && !active_orders.empty()) {
            // Cancel random order
            std::uniform_int_distribution<> idx_dist(0, active_orders.size() - 1);
            int idx = idx_dist(rng);
            OrderId to_cancel = active_orders[idx];
            active_orders.erase(active_orders.begin() + idx);
            
            CancelRequest cancel;
            cancel.order_id = to_cancel;
            cancel.symbol = symbols[to_cancel % symbols.size()];
            cancel.timestamp = i * 1000;
            
            engine.handle(cancel);
        } else {
            // New order around mid price with spread
            NewOrderRequest req;
            req.order_id = next_order_id++;
            req.trader_id = trader_dist(rng);
            req.symbol = symbols[i % symbols.size()];
            req.side = side_dist(rng) == 0 ? Side::BUY : Side::SELL;
            req.order_type = OrderType::LIMIT;
            
            int spread_offset = spread_dist(rng);
            req.price = mid_price + (req.side == Side::BUY ? -spread_offset : spread_offset);
            req.quantity = qty_dist(rng);
            req.time_in_force = TimeInForce::DAY;
            req.timestamp = i * 1000;
            
            auto response = engine.handle(req);
            
            if (response.result == ResultCode::SUCCESS) {
                active_orders.push_back(req.order_id);
            }
        }
        
        // Drift mid price slightly
        if (i % 1000 == 0) {
            std::uniform_int_distribution<> drift_dist(-5, 5);
            mid_price += drift_dist(rng);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    // Results
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double seconds = duration / 1000000.0;
    double throughput = NUM_ORDERS / seconds;
    
    std::cout << "\n=== Results ===\n";
    std::cout << "Total actions:    " << NUM_ORDERS << "\n";
    std::cout << "Total time:       " << std::fixed << std::setprecision(3) 
              << seconds << " seconds\n";
    std::cout << "Throughput:       " << std::fixed << std::setprecision(0) 
              << throughput << " actions/sec\n";
    
    std::cout << "\nEngine telemetry:\n";
    std::cout << engine.get_telemetry_json().dump(2) << "\n";
    
    return 0;
}

