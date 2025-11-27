# Design & Architecture

## Overview

This document describes the design and architecture of the HFT Limit Order Book Matching Engine. The system implements a production-style matching engine with real exchange semantics, deterministic replay, and comprehensive validation.

## Core Components

### 1. OrderBook

**Purpose**: Manages a single-symbol limit order book with price-time priority matching.

**Data Structures**:
- **Price Levels**: `std::map<Price, PriceLevelQueue>` for O(log N) insertion/deletion
  - Bids: Descending order (`std::greater<Price>`) - highest first
  - Asks: Ascending order (`std::less<Price>`) - lowest first
- **FIFO Queues**: `std::deque<Order*>` at each price level
- **Order Lookup**: `std::unordered_map<OrderId, unique_ptr<Order>>` for O(1) cancel/replace

**Matching Algorithm**:
```
For incoming BUY order:
  1. While remaining_qty > 0 and best_ask exists and order.price >= best_ask:
     2. Match against front of ask queue (FIFO)
     3. Generate trade event
     4. Update quantities
     5. Remove fully filled orders
     6. Remove empty price levels
  7. If remaining_qty > 0 and order is not IOC/FOK:
     8. Add to bid side at order.price

For incoming SELL order: (symmetric logic with bids)
```

**Complexity**:
- Best bid/ask: O(1)
- Add order: O(log N) price levels + O(M) matching iterations
- Cancel order: O(1) lookup + O(log N) removal from price level
- Replace order: Cancel + Add

### 2. ReferenceOrderBook

**Purpose**: Naive O(N²) reference implementation for validation.

**Design Philosophy**: Deliberately simple, easy to verify correctness.

**Data Structure**: Single `vector<unique_ptr<Order>>` with linear search.

**Matching**: For each incoming order, linearly search all opposite-side orders, find best match by price then time, repeat until filled or no more matches.

**Use Cases**:
- Unit test oracle
- Fuzz test validation
- Debugging mismatches

### 3. MatchingEngine

**Purpose**: Multi-symbol routing, symbol configuration, event coordination.

**Architecture**:
```
MatchingEngine
├── symbol_configs_: map<symbol, SymbolConfig>
├── order_books_: map<symbol, unique_ptr<OrderBook>>
├── trade_tapes_: map<symbol, unique_ptr<TradeTape>>
├── event_log_: EventLog (deterministic mode)
├── telemetry_: Telemetry (metrics)
└── listeners_: vector<shared_ptr<IMatchingEngineListener>>
```

**Request Handling Flow**:
```
1. Receive NewOrderRequest
2. Validate (symbol exists, price/quantity valid)
3. Route to appropriate OrderBook
4. Record incoming message (if deterministic)
5. Execute matching
6. Collect trade events
7. Update telemetry
8. Notify listeners
9. Log events (if deterministic)
10. Return OrderResponse
```

### 4. EventLog (Deterministic Mode)

**Purpose**: Record all inputs and outputs for exact replay.

**Format**: JSON lines format
```
{"type": "NEW_ORDER", "seq": 1, "ts": 123456789, "data": {...}}
{"type": "TRADE", "seq": 2, "ts": 123456790, "data": {...}}
```

**Guarantees**:
- Monotonic sequence numbers
- Buffered writes (flush on demand)
- Replay produces bit-identical results

**Use Cases**:
- Backtesting
- Debugging production issues
- Regulatory compliance
- Simulation reproducibility

### 5. Telemetry

**Metrics Tracked**:
- Engine-wide:
  - Orders processed/accepted/rejected/cancelled
  - Total trades
  - Latency distribution (min/avg/max)
- Per-symbol:
  - Active orders
  - Price levels (bid/ask)
  - Trade volume/count
  - Max depth

**Export**: JSON format for integration with monitoring systems.

### 6. Market Data Views

**TopOfBook**: Best bid/ask with sizes, mid-price, spread.

**DepthSnapshot**: Top-N price levels with aggregated quantities.

**Serialization Formats**:
1. **JSON**: Human-readable, debugging
2. **CSV**: Simple parsing, trade prints
3. **Binary**: Zero-copy, performance-critical market data feeds

**Binary Format**:
```
[Header: 32 bytes]
  - magic: 0xLOB1 (4 bytes)
  - version: 1 (2 bytes)
  - symbol_len: N (2 bytes)
  - num_bids: M (4 bytes)
  - num_asks: K (4 bytes)
  - timestamp: T (8 bytes)
  - sequence: S (8 bytes)
  - checksum: CRC32 (4 bytes)

[Symbol: N bytes]

[Bid Levels: M * 16 bytes]
  - price: int64 (8 bytes)
  - quantity: uint64 (8 bytes)

[Ask Levels: K * 16 bytes]
  - price: int64 (8 bytes)
  - quantity: uint64 (8 bytes)
```

### 7. Lock-Free Market Data Publisher

**Purpose**: Decouple matching from market data publishing.

**Design**: Single-producer single-consumer (SPSC) lock-free ring buffer.

**Implementation**:
- Cache-line aligned atomics (64 bytes)
- Power-of-2 capacity for efficient modulo
- Wait-free enqueue/dequeue
- Separate consumer thread

**Benefit**: Zero contention between matching engine and market data consumers.

## Order Types & Semantics

### LIMIT Order
- Rests on book at specified price
- Matches if price crosses spread
- Supports partial fills

### MARKET Order
- Executes immediately at best available prices
- Walks the book until filled or exhausted
- Never rests on book

### IOC (Immediate-Or-Cancel)
- Matches immediately
- Any unfilled remainder is cancelled
- Never rests on book

### FOK (Fill-Or-Kill)
- Pre-check: Is sufficient liquidity available?
- If yes: execute atomically
- If no: reject entire order
- Never partially filled

### Cancel
- Remove by OrderId
- O(1) lookup + O(log N) removal

### Replace/Amend
- Cancel-replace semantics
- Time priority rules:
  - Price change → lose time priority (new order)
  - Quantity reduction at same price → keep time priority
  - Quantity increase at same price → configurable (default: lose priority)

## Self-Trade Prevention

**Problem**: Same trader matching against own orders.

**Policies**:
1. **CANCEL_INCOMING**: Cancel incoming order
2. **CANCEL_RESTING**: Cancel resting order
3. **CANCEL_BOTH**: Cancel both orders

**Implementation**: Check `trader_id` before generating trade.

## Validation Strategy

**Dual-Engine Testing**:
```
For random order stream:
  1. Feed to optimized OrderBook
  2. Feed to ReferenceOrderBook
  3. Compare trade sequences
  4. Compare final book states
  5. Report mismatches
```

**Advantages**:
- High confidence in correctness
- Catches subtle bugs (off-by-one, ordering issues)
- Industry-standard practice (Citadel, HRT use this)

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Best bid/ask | O(1) | Direct map access |
| Add order (no match) | O(log N) | Insert into map |
| Add order (matching) | O(log N + M) | M = orders matched |
| Cancel order | O(1) + O(log N) | Lookup + map removal |
| Replace order | O(log N) | Cancel + Add |
| Top-of-book snapshot | O(1) | Direct access |
| Depth snapshot (N levels) | O(N) | Iterate N levels |

**Memory**:
- Per order: ~200 bytes (Order struct + pointers + map overhead)
- Per price level: ~64 bytes (PriceLevelQueue + map node)
- Depth snapshot (10 levels): ~500 bytes

**Expected Throughput**:
- Optimized: >100K orders/sec single-threaded
- Reference: ~1K orders/sec (validation only)

## Thread Safety

**Current Design**: Single-threaded matching core (deterministic, no locks).

**Future Multi-Threading**:
- Lock-free order submission queues (per-symbol)
- Sharded order books by symbol
- Lock-free market data publishing (implemented)

## Extension Points

1. **Risk Management**: Pre-trade risk checks (position limits, notional limits)
2. **Advanced Order Types**: Iceberg, hidden, pegged, stop-loss
3. **Auction Mode**: Opening/closing auctions, price discovery
4. **Market Maker Protections**: Quote width requirements, maker rebates
5. **Networking Layer**: FIX protocol gateway, WebSocket API
6. **Persistence**: WAL for crash recovery

## Trade-Offs & Decisions

### Why std::map for price levels?
- **Pro**: O(log N) operations, best bid/ask in O(1), clean iteration
- **Con**: Slower than hash table for random access
- **Decision**: Price levels are naturally ordered; iteration is common

### Why std::deque for FIFO queue?
- **Pro**: Efficient front/back operations, stable iterators
- **Con**: Not cache-friendly for iteration
- **Alternative**: Intrusive linked list (more complex, ~10% faster)

### Why unique_ptr for orders?
- **Pro**: Clear ownership, no manual delete, exception-safe
- **Con**: Heap allocation per order
- **Alternative**: Object pool (optimization opportunity)

### Why JSON for event log?
- **Pro**: Human-readable, debuggable, schemaless
- **Con**: Slower than binary
- **Decision**: Determinism > performance for logging

### Why reference implementation?
- **Pro**: Confidence in correctness, catches regression bugs
- **Con**: Extra code to maintain
- **Decision**: Correctness is paramount; reference is simple enough

## Diagrams

### Order Matching Flow

```
Incoming Order
     │
     ├──> Validate
     │      │
     │      ├──> Invalid ──> Reject
     │      │
     │      └──> Valid
     │            │
     ├──────────> Match Against Book
     │                  │
     │                  ├──> Generate Trades
     │                  │        │
     │                  │        └──> Update Quantities
     │                  │
     │                  └──> Check TIF
     │                         │
     │                         ├──> IOC ──> Cancel Remainder
     │                         ├──> FOK ──> Reject if not fully filled
     │                         └──> DAY/GTC ──> Add to Book
     │
     └──> Log Events ──> Notify Listeners ──> Return Response
```

### Book Structure

```
OrderBook (AAPL)
│
├── Bids (map, descending)
│   ├── 100.50 → [Order1, Order2, Order3]  ← FIFO queue
│   ├── 100.49 → [Order4]
│   └── 100.48 → [Order5, Order6]
│
├── Asks (map, ascending)
│   ├── 100.51 → [Order7]                   ← FIFO queue
│   ├── 100.52 → [Order8, Order9]
│   └── 100.53 → [Order10]
│
└── Orders (unordered_map)
    ├── OrderId1 → unique_ptr<Order1>
    ├── OrderId2 → unique_ptr<Order2>
    └── ...
```

## Conclusion

This design balances:
- **Correctness**: Dual-engine validation, comprehensive tests
- **Performance**: Efficient data structures, minimal allocations
- **Clarity**: Clean interfaces, well-documented
- **Production-Readiness**: Determinism, telemetry, extensibility

The architecture demonstrates real-world HFT engineering practices suitable for showcasing in interviews at top-tier quantitative trading firms.

