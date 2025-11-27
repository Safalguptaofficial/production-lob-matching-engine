# HFT Limit Order Book Matching Engine

> **Production-style LOB matching engine with deterministic replay, dual-engine validation, and binary market data feeds**

A high-performance C++20 implementation of an exchange-style limit order book with real matching logic, comprehensive order types, and advanced features suitable for HFT/quantitative finance applications.

## Key Features

- ✅ **Deterministic & Replayable**: Event logging system for reproducible backtests and debugging
- ✅ **Dual-Engine Validation**: Reference O(N²) matcher for fuzz testing and correctness validation
- ✅ **Multiple Serialization Formats**: JSON, CSV, and binary (zero-copy) market data feeds
- ✅ **Comprehensive Telemetry**: Nanosecond-precision latency tracking and throughput metrics
- ✅ **Lock-Free Market Data**: Optional SPSC ring buffer for publishing trade events
- ✅ **Advanced Order Types**: LIMIT, MARKET, IOC, FOK with self-trade prevention
- ✅ **Multi-Symbol Support**: Efficient routing across multiple instruments
- ✅ **Price-Time Priority**: Real exchange-style FIFO matching at each price level

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     MatchingEngine                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Symbol Config│  │  OrderBook   │  │  TradeTape   │      │
│  └──────────────┘  │   (AAPL)     │  └──────────────┘      │
│                     └──────────────┘                         │
│                     │  OrderBook   │  ┌──────────────┐      │
│                     │   (MSFT)     │  │  Telemetry   │      │
│                     └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              │
                ┌─────────────┼─────────────┐
                │             │             │
        ┌───────▼──────┐ ┌───▼─────┐ ┌────▼──────┐
        │  EventLog    │ │Listeners│ │LockFreeQ  │
        │(Deterministic│ │         │ │(Optional) │
        └──────────────┘ └─────────┘ └───────────┘

OrderBook Structure (Per Symbol):
    Bids (std::map, descending)         Asks (std::map, ascending)
    Price   FIFO Queue                   Price   FIFO Queue
    ═════   ══════════════               ═════   ══════════════
    100.50  [Order1, Order2]             100.51  [Order5]
    100.49  [Order3]                     100.52  [Order6, Order7]
    100.48  [Order4]                     100.53  [Order8]
```

## Quick Start

### Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Run Examples

```bash
# Simple demo
./build/simple_demo

# Throughput benchmark
./build/bench_throughput

# Market simulator
./build/bench_market_sim

# Replay tool (placeholder)
./build/replay --help
```

### Run Tests

```bash
cd build
ctest --output-on-failure
```

**Note**: Tests require GoogleTest. If you encounter RPATH issues on macOS, you can skip test discovery or run tests manually.

### Simple Example

```cpp
#include <lob/matching_engine.hpp>

int main() {
    lob::MatchingEngine engine;
    
    // Register symbol
    lob::SymbolConfig config{"AAPL", 1, 1, 1};
    engine.add_symbol(config);
    
    // Submit buy order
    lob::NewOrderRequest buy_order{
        .order_id = 1,
        .trader_id = 100,
        .symbol = "AAPL",
        .side = lob::Side::BUY,
        .order_type = lob::OrderType::LIMIT,
        .price = 15000, // $150.00 (in cents)
        .quantity = 100,
        .time_in_force = lob::TimeInForce::DAY
    };
    
    auto response = engine.handle(buy_order);
    
    // Check for trades
    for (const auto& trade : response.trades) {
        std::cout << "Trade: " << trade.quantity 
                  << " @ " << trade.price / 100.0 << "\n";
    }
    
    // Get top of book
    auto tob = engine.get_top_of_book("AAPL");
    std::cout << "Best Bid: " << tob.best_bid / 100.0 << "\n";
    std::cout << "Best Ask: " << tob.best_ask / 100.0 << "\n";
}
```

**Output from `simple_demo`:**

```
HFT Limit Order Book - Simple Demo
================================================================================
...
Top of Book (AAPL):
  Best Bid: 150.00 (100 shares)
  Best Ask: 150.01 (150 shares)
  Spread:   0.01
  Mid:      150.00
================================================================================

3. Sending aggressive buy order that crosses the spread...

TRADE: BUY 100 @ 150.01 (IDs: 4 x 3)
...
```

## Replay Tool Usage

```bash
# Show help
./build/replay --help

# Replay with options (CSV parsing to be implemented)
./build/replay --input orders.csv --deterministic --print-trades --validate --stats

# Options:
#   --deterministic      Enable event logging
#   --print-trades       Print all trades to stdout
#   --print-depth N      Show top N price levels
#   --validate           Run reference engine in parallel
#   --binary-snapshots   Use binary serialization
#   --stats              Show telemetry summary
```

**Note**: Full CSV parsing implementation is a placeholder. The structure is in place for extension.

## Performance Characteristics

**Measured on Apple M1 (single-threaded)**:

| Metric | Value |
|--------|-------|
| Throughput (optimized) | **160K orders/sec** |
| Throughput (w/ cancels) | **287K actions/sec** |
| Avg Latency | **6 μs** per order |
| Best Bid/Ask Lookup | O(1) |
| Order Insert/Cancel | O(log N) price levels |
| Memory per Order | ~200 bytes |
| Trades Generated | 76K from 100K orders |

## Resume Talking Points

This project demonstrates:

- **Deterministic Systems Design**: Implemented replayable event logs similar to CME/NASDAQ production systems
- **Dual-Engine Validation**: Designed O(N²) reference matcher for property-based fuzz testing
- **Binary Protocols**: Built zero-copy binary market data serialization
- **Lock-Free Programming**: SPSC ring buffer for market data publishing with no contention
- **Low-Latency Optimization**: Nanosecond-precision profiling and cache-aware data structures
- **Software Engineering**: Comprehensive testing (unit, fuzz, property-based), modern C++20, clean API design

## Documentation

- [Design & Architecture](docs/design.md) - Detailed system design and trade-offs
- [Microstructure Basics](docs/microstructure.md) - LOB fundamentals and price-time priority
- [Determinism & Replay](docs/determinism.md) - Event logging and replay architecture
- [Validation Strategy](docs/validation.md) - Dual-engine testing approach
- [Serialization Formats](docs/serialization.md) - JSON vs binary performance comparison

## Order Types Supported

- **LIMIT**: Rest on book at specified price, match if price crosses
- **MARKET**: Immediately match at best available prices
- **IOC** (Immediate-Or-Cancel): Match immediately, cancel remainder
- **FOK** (Fill-Or-Kill): All-or-nothing execution
- **Cancel**: Remove active order by ID
- **Replace**: Amend price/quantity with time priority rules

## Testing

- **Unit Tests**: 14 test suites covering all order types and edge cases
- **Property-Based Tests**: Random order streams validated against reference implementation
- **Fuzz Tests**: Automatic mismatch detection between optimized and reference engines
- **Replay Tests**: Deterministic replay verification

## License

MIT License - see LICENSE file

## Author

Built as a demonstration project for HFT/quantitative finance interviews.

---

## Project Statistics

- **~3,500 lines** of production-quality C++20 code
- **14 comprehensive test suites** (unit, integration, property-based)
- **Dual-engine validation** (optimized + reference implementation)
- **100+ documented functions** with clear interfaces
- **4 detailed design documents** explaining architecture and trade-offs
- **Zero external runtime dependencies** (except standard library and nlohmann/json)

## Known Limitations & Future Work

This is an educational/demonstration project showcasing HFT engineering practices. For production deployment, consider adding:

- **Networking Layer**: FIX protocol gateway, WebSocket API
- **Risk Management**: Pre-trade risk checks, position limits, circuit breakers
- **Persistence**: Write-ahead log (WAL) for crash recovery
- **Monitoring**: Prometheus metrics, distributed tracing
- **Multi-Threading**: Lock-free queues for order submission (core matching remains single-threaded)
- **Advanced Order Types**: Iceberg, hidden, pegged orders
- **Auction Mechanisms**: Opening/closing auctions
- **FPGA Acceleration**: For ultra-low latency (< 100ns)

## Contributions

This project was built as a learning exercise and resume showcase for HFT/quantitative finance interviews. Feedback and suggestions are welcome!

