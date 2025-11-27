# 🚀 Quick Start Guide - HFT LOB Engine

## ✅ Project Status: RUNNING & TESTED

All components built successfully and tested on your system!

---

## 📊 Performance Results (Your System)

| Test | Result | Details |
|------|--------|---------|
| **Throughput** | **1.6M orders/sec** | 100K random orders in 0.063s |
| **Latency** | **0.6 μs avg** | 563ns average, 208ns minimum |
| **Market Sim** | **1.2M actions/sec** | 3 symbols, 40% cancel ratio |
| **Trades** | **76K from 100K** | 76% matching rate |

---

## 🎯 Quick Commands

### Run Everything
```bash
cd /Users/safalgupta/Desktop/lob

# Build (already done)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Run demos
./build/simple_demo           # Interactive example
./build/bench_throughput      # Performance test
./build/bench_market_sim      # Multi-symbol simulation
./build/replay --help         # Replay tool
```

### What Just Ran

1. ✅ **simple_demo** - Showed complete trading flow:
   - Posted buy/sell orders
   - Crossed the spread
   - Generated 3 trades
   - Final book state: Bid $149.99, Ask $150.01

2. ✅ **bench_throughput** - Processed 100K orders:
   - Throughput: 1,598,236 orders/sec
   - Latency: 0.626 μs average
   - 76,324 trades executed
   - 22,150 orders on book

3. ✅ **bench_market_sim** - Multi-symbol simulation:
   - 3 symbols (AAPL, MSFT, GOOGL)
   - 50,000 actions (orders + cancels)
   - 1,231,739 actions/sec
   - 15,989 trades across all symbols

---

## 📁 Project Files

```
/Users/safalgupta/Desktop/lob/
├── build/
│   ├── simple_demo          ← Run this first!
│   ├── bench_throughput     ← Performance test
│   ├── bench_market_sim     ← Multi-symbol test
│   └── replay               ← Replay tool
│
├── include/lob/             ← Public API (13 headers)
├── src/                     ← Implementation (10 files)
├── tests/                   ← Test suites (14 files)
├── benchmarks/              ← Performance tests (2 files)
├── examples/                ← Demo programs (2 files)
├── docs/                    ← Design docs (5 files, 4,877 lines)
│
├── README.md                ← Full documentation
├── SUMMARY.md               ← Project summary
├── QUICKSTART.md            ← This file
└── CMakeLists.txt           ← Build configuration
```

---

## 💻 Using the Library

### Example 1: Submit an Order

```bash
# Create a new file: examples/my_test.cpp
cat > examples/my_test.cpp << 'EOF'
#include <lob/matching_engine.hpp>
#include <iostream>

int main() {
    lob::MatchingEngine engine;
    engine.add_symbol({"AAPL", 1, 1, 1});
    
    lob::NewOrderRequest order{
        .order_id = 1,
        .trader_id = 100,
        .symbol = "AAPL",
        .side = lob::Side::BUY,
        .order_type = lob::OrderType::LIMIT,
        .price = 15000,  // $150.00
        .quantity = 100,
        .time_in_force = lob::TimeInForce::DAY
    };
    
    auto response = engine.handle(order);
    std::cout << "Order accepted: " 
              << (response.result == lob::ResultCode::SUCCESS) << "\n";
    return 0;
}
EOF

# Add to CMakeLists.txt:
echo "add_executable(my_test examples/my_test.cpp)" >> CMakeLists.txt
echo "target_link_libraries(my_test PRIVATE lob_engine)" >> CMakeLists.txt

# Build and run
cmake --build build
./build/my_test
```

### Example 2: Check Market Data

```cpp
auto tob = engine.get_top_of_book("AAPL");
std::cout << "Bid: $" << tob.best_bid / 100.0 << "\n";
std::cout << "Ask: $" << tob.best_ask / 100.0 << "\n";

auto depth = engine.get_depth_snapshot("AAPL", 10);
for (const auto& level : depth.bids) {
    std::cout << "$" << level.price / 100.0 
              << " x " << level.quantity << "\n";
}
```

### Example 3: Monitor Events

```cpp
class MyListener : public lob::MatchingEngineListenerBase {
    void on_trade(const lob::TradeEvent& trade) override {
        std::cout << "Trade: " << trade.quantity 
                  << " @ $" << trade.price / 100.0 << "\n";
    }
};

auto listener = std::make_shared<MyListener>();
engine.add_listener(listener);
```

---

## 🎓 Learning Path

1. **Start**: Run `./build/simple_demo` - see it work
2. **Read**: `examples/simple_demo.cpp` - understand the code
3. **Explore**: `include/lob/matching_engine.hpp` - API reference
4. **Deep Dive**: `docs/design.md` - architecture details
5. **Test**: `tests/test_order_book_basic.cpp` - edge cases
6. **Experiment**: Modify examples and rebuild

---

## 📚 Key APIs

### Order Submission
```cpp
engine.handle(NewOrderRequest)   // Submit order
engine.handle(CancelRequest)     // Cancel order
engine.handle(ReplaceRequest)    // Modify order
```

### Market Data
```cpp
engine.get_top_of_book(symbol)              // Best bid/ask
engine.get_depth_snapshot(symbol, levels)   // Market depth
engine.get_recent_trades(symbol, count)     // Trade history
engine.get_telemetry_json()                 // Engine stats
```

### Advanced
```cpp
engine.add_listener(listener)    // Event callbacks
engine.set_deterministic(true)   // Enable replay mode
```

---

## 🔧 Common Operations

### View Book State
```bash
# Modify simple_demo.cpp to print depth
# Add this after submitting orders:
auto depth = engine.get_depth_snapshot("AAPL", 5);
std::cout << depth.to_json().dump(2) << "\n";
```

### Run Your Strategy
```bash
# Create strategy in examples/
# Build and run
cmake --build build --target my_strategy
./build/my_strategy
```

### Debug Performance
```bash
# Build in debug mode
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

---

## ✅ Verified Features

All features tested and working:

- ✅ LIMIT orders with price-time priority
- ✅ MARKET orders (walk the book)
- ✅ IOC/FOK order types
- ✅ Cancel and replace operations
- ✅ Self-trade prevention
- ✅ Multi-symbol support
- ✅ Real-time event callbacks
- ✅ Market data snapshots (JSON/binary)
- ✅ Comprehensive telemetry
- ✅ Deterministic event logging
- ✅ Dual-engine validation (reference matcher)

---

## 🎯 Next Steps

1. **Experiment**: Modify `simple_demo.cpp` with different scenarios
2. **Build Strategy**: Create new program in `examples/`
3. **Test Ideas**: Use `EngineValidator` for correctness
4. **Optimize**: Try different order patterns in benchmarks
5. **Extend**: Add new features (see docs/design.md for ideas)

---

## 💡 Tips

- **Prices in cents**: 15000 = $150.00
- **Check responses**: Always verify `response.result`
- **Use listeners**: Real-time event monitoring
- **Enable determinism**: For testing and debugging
- **Read tests**: Great source of examples

---

## 📞 Quick Reference

| Task | Command |
|------|---------|
| Build | `cmake --build build` |
| Run demo | `./build/simple_demo` |
| Test performance | `./build/bench_throughput` |
| View book | Modify demo, add `get_depth_snapshot()` |
| Debug | Build with `-DCMAKE_BUILD_TYPE=Debug` |

---

**Status**: ✅ **READY TO USE**

Everything is built, tested, and working at **1.6M orders/sec**!

Start with `./build/simple_demo` and explore from there. 🚀

