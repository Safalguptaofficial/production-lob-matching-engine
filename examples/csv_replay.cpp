// Real market data replay from CSV files
#include <lob/matching_engine.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>

struct CSVOrder {
    uint64_t timestamp;
    std::string symbol;
    std::string side;      // BUY or SELL
    std::string order_type; // LIMIT or MARKET
    double price;
    uint64_t quantity;
    uint64_t order_id;
    uint64_t trader_id;
};

CSVOrder parse_csv_line(const std::string& line) {
    CSVOrder order;
    std::stringstream ss(line);
    std::string token;
    
    std::getline(ss, token, ','); order.timestamp = std::stoull(token);
    std::getline(ss, order.symbol, ',');
    std::getline(ss, order.side, ',');
    std::getline(ss, order.order_type, ',');
    std::getline(ss, token, ','); order.price = std::stod(token);
    std::getline(ss, token, ','); order.quantity = std::stoull(token);
    std::getline(ss, token, ','); order.order_id = std::stoull(token);
    std::getline(ss, token, ','); order.trader_id = std::stoull(token);
    
    return order;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <csv_file>\n";
        std::cerr << "\nCSV Format (no header):\n";
        std::cerr << "timestamp,symbol,side,order_type,price,quantity,order_id,trader_id\n";
        std::cerr << "\nExample:\n";
        std::cerr << "1638360000000,AAPL,BUY,LIMIT,150.25,100,1,1001\n";
        std::cerr << "1638360001000,AAPL,SELL,LIMIT,150.26,50,2,1002\n";
        return 1;
    }
    
    std::string csv_file = argv[1];
    std::ifstream file(csv_file);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << csv_file << "\n";
        return 1;
    }
    
    std::cout << "=== Real Market Data Replay ===\n";
    std::cout << "Loading orders from: " << csv_file << "\n\n";
    
    // Create matching engine
    lob::MatchingEngine engine(true);  // Deterministic mode
    
    // Track symbols we've seen
    std::set<std::string> registered_symbols;
    
    std::string line;
    int line_num = 0;
    int orders_processed = 0;
    int trades_executed = 0;
    
    // Skip header if present
    std::getline(file, line);
    if (line.find("timestamp") == std::string::npos) {
        // Not a header, process as data
        file.seekg(0);
    }
    
    while (std::getline(file, line)) {
        line_num++;
        
        if (line.empty()) continue;
        
        try {
            CSVOrder csv_order = parse_csv_line(line);
            
            // Register symbol if not seen before
            if (registered_symbols.find(csv_order.symbol) == registered_symbols.end()) {
                lob::SymbolConfig config{csv_order.symbol, 1, 1, 1};
                engine.add_symbol(config);
                registered_symbols.insert(csv_order.symbol);
                std::cout << "Registered symbol: " << csv_order.symbol << "\n";
            }
            
            // Convert to engine request
            lob::NewOrderRequest request{
                .order_id = csv_order.order_id,
                .trader_id = csv_order.trader_id,
                .symbol = csv_order.symbol,
                .side = csv_order.side == "BUY" ? lob::Side::BUY : lob::Side::SELL,
                .order_type = csv_order.order_type == "MARKET" ? 
                              lob::OrderType::MARKET : lob::OrderType::LIMIT,
                .price = static_cast<lob::Price>(csv_order.price * 100), // Convert to cents
                .quantity = csv_order.quantity,
                .time_in_force = lob::TimeInForce::DAY,
                .timestamp = csv_order.timestamp
            };
            
            // Process order
            auto response = engine.handle(request);
            orders_processed++;
            
            if (response.result == lob::ResultCode::SUCCESS) {
                trades_executed += response.trades.size();
                
                // Print trades
                for (const auto& trade : response.trades) {
                    std::cout << "TRADE [" << csv_order.symbol << "] "
                              << trade.quantity << " @ $" 
                              << trade.price / 100.0 << "\n";
                }
            }
            
            // Print progress every 1000 orders
            if (orders_processed % 1000 == 0) {
                std::cout << "Progress: " << orders_processed << " orders processed, "
                          << trades_executed << " trades\n";
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line " << line_num << ": " << e.what() << "\n";
            continue;
        }
    }
    
    std::cout << "\n=== Replay Complete ===\n";
    std::cout << "Orders processed: " << orders_processed << "\n";
    std::cout << "Trades executed: " << trades_executed << "\n";
    std::cout << "Symbols: " << registered_symbols.size() << "\n";
    
    // Print final book states
    std::cout << "\n=== Final Book States ===\n";
    for (const auto& symbol : registered_symbols) {
        auto tob = engine.get_top_of_book(symbol);
        std::cout << symbol << ": ";
        if (tob.best_bid != lob::INVALID_PRICE) {
            std::cout << "Bid $" << tob.best_bid / 100.0 << " (" << tob.bid_size << ")";
        }
        if (tob.best_ask != lob::INVALID_PRICE) {
            std::cout << " | Ask $" << tob.best_ask / 100.0 << " (" << tob.ask_size << ")";
        }
        std::cout << "\n";
    }
    
    // Save telemetry
    std::cout << "\n=== Engine Statistics ===\n";
    std::cout << engine.get_telemetry_json().dump(2) << "\n";
    
    return 0;
}
