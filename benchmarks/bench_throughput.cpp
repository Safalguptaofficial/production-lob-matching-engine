// Throughput benchmark for LOB matching engine
#include <lob/matching_engine.hpp>
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace lob;

int main() {
    std::cout << "=== LOB Throughput Benchmark ===\n\n";
    
    // Setup
    const int NUM_ORDERS = 100000;
    std::mt19937 rng(42);
    std::uniform_int_distribution<> side_dist(0, 1);
    std::uniform_int_distribution<> price_dist(9900, 10100);
    std::uniform_int_distribution<> qty_dist(10, 1000);
    std::uniform_int_distribution<> trader_dist(100, 150);
    
    MatchingEngine engine;
    SymbolConfig config{"TEST", 1, 1, 1};
    engine.add_symbol(config);
    
    std::cout << "Generating " << NUM_ORDERS << " random orders...\n";
    
    // Pre-generate orders
    std::vector<NewOrderRequest> orders;
    orders.reserve(NUM_ORDERS);
    
    for (int i = 0; i < NUM_ORDERS; ++i) {
        NewOrderRequest req;
        req.order_id = i + 1;
        req.trader_id = trader_dist(rng);
        req.symbol = "TEST";
        req.side = side_dist(rng) == 0 ? Side::BUY : Side::SELL;
        req.order_type = OrderType::LIMIT;
        req.price = price_dist(rng);
        req.quantity = qty_dist(rng);
        req.time_in_force = TimeInForce::DAY;
        req.timestamp = i * 1000;
        
        orders.push_back(req);
    }
    
    std::cout << "Running benchmark...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (const auto& order : orders) {
        engine.handle(order);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    // Results
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double seconds = duration / 1000000.0;
    double throughput = NUM_ORDERS / seconds;
    double avg_latency_us = duration / static_cast<double>(NUM_ORDERS);
    
    std::cout << "\n=== Results ===\n";
    std::cout << "Total orders:     " << NUM_ORDERS << "\n";
    std::cout << "Total time:       " << std::fixed << std::setprecision(3) 
              << seconds << " seconds\n";
    std::cout << "Throughput:       " << std::fixed << std::setprecision(0) 
              << throughput << " orders/sec\n";
    std::cout << "Avg latency:      " << std::fixed << std::setprecision(3) 
              << avg_latency_us << " μs/order\n";
    
    std::cout << "\nEngine telemetry:\n";
    std::cout << engine.get_telemetry_json().dump(2) << "\n";
    
    return 0;
}

