# Deterministic Mode & Replay Architecture

## Why Determinism Matters in HFT

### Problem: Non-Deterministic Systems

In typical software, behavior can vary between runs due to:
- Thread scheduling non-determinism
- Memory allocation order
- Hash table iteration order
- Floating-point rounding differences
- System time variations

### Why This is Fatal for Trading Systems

1. **Regulatory Compliance**: 
   - Regulators (SEC, CFTC) require trade reconstruction
   - Must prove what happened during market events
   - Non-determinism = no audit trail

2. **Backtesting**:
   - Strategy backtests must be reproducible
   - Can't iterate on alpha if results differ each run
   - "It worked yesterday" is unacceptable

3. **Debugging**:
   - Production bugs must be reproducible
   - Replaying 1 billion orders to find bug is only feasible if deterministic
   - Time-travel debugging requires determinism

4. **Compliance & Litigation**:
   - Flash crash investigations require exact replay
   - "Why did your algorithm do X?" must have provable answer
   - Legal liability if behavior is unexplainable

### Real-World Examples

**CME**: All orders and fills logged with microsecond timestamps, replayable for audit.

**NASDAQ**: ITCH feed provides total market transparency, complete reconstruction possible.

**Jane Street / Citadel**: Internal replay systems for every trading decision (I know from interviews).

## Our Deterministic Design

### Core Principles

1. **Single-Threaded Matching**: No race conditions in core order book
2. **Explicit Timestamps**: All events have monotonic sequence numbers
3. **Complete Event Logging**: Every input and output is recorded
4. **Idempotent Operations**: Replaying same inputs produces identical outputs
5. **No Hidden State**: All state is derived from event log

### Event Log Format

**File Format**: JSON Lines (newline-delimited JSON)

**Example**:
```json
{"type":"NEW_ORDER","seq":1,"ts":1234567890,"data":{"order_id":100,"trader_id":1,"symbol":"AAPL","side":"BUY","price":15000,"quantity":100}}
{"type":"ORDER_ACCEPTED","seq":2,"ts":1234567891,"data":{"order_id":100,"symbol":"AAPL"}}
{"type":"NEW_ORDER","seq":3,"ts":1234567900,"data":{"order_id":101,"trader_id":2,"symbol":"AAPL","side":"SELL","price":15000,"quantity":50}}
{"type":"TRADE","seq":4,"ts":1234567901,"data":{"trade_id":1,"symbol":"AAPL","price":15000,"quantity":50,"aggressive_order_id":101,"passive_order_id":100}}
{"type":"ORDER_ACCEPTED","seq":5,"ts":1234567902,"data":{"order_id":100,"symbol":"AAPL"}}
```

**Fields**:
- `type`: Event type (NEW_ORDER, CANCEL, TRADE, etc.)
- `seq`: Monotonic sequence number (gap indicates lost events)
- `ts`: Timestamp (nanoseconds since epoch, or logical clock)
- `data`: Event-specific payload (JSON object)

### Event Types Logged

**Inputs** (Commands):
- `NEW_ORDER`: NewOrderRequest
- `CANCEL`: CancelRequest
- `REPLACE`: ReplaceRequest

**Outputs** (Events):
- `ORDER_ACCEPTED`: Order added to book
- `ORDER_REJECTED`: Order rejected (with reason)
- `ORDER_CANCELLED`: Order removed
- `ORDER_REPLACED`: Order amended
- `TRADE`: Match occurred

**Metadata** (Optional):
- `SNAPSHOT`: Periodic book state checkpoint
- `HEARTBEAT`: Liveness indicator

### Replay Process

```
1. Load event log from disk
2. Create fresh MatchingEngine instance
3. For each logged event:
   a. If input (NEW_ORDER, CANCEL, REPLACE):
      - Submit to engine
   b. If output (TRADE, ORDER_ACCEPTED, etc.):
      - Compare with engine's actual output
      - Assert exact match
4. Compare final book state
5. Report any mismatches
```

### Guarantees

**Determinism Guarantee**: Given same event log, replay produces:
- Identical trade sequence (trade_id, price, quantity, order_ids)
- Identical final book state (all price levels, all orders)
- Identical reject reasons

**What is NOT guaranteed**:
- Absolute timestamps (can vary due to wall clock skew)
- Memory addresses (ASLR)
- Hash table iteration order (we use ordered containers)

## Implementation Details

### EventLog Class

**Key Methods**:
```cpp
// Enable deterministic mode
void set_deterministic(bool enabled);

// Log incoming requests
void log_new_order(const NewOrderRequest& request);
void log_cancel(const CancelRequest& request);

// Log resulting events
void log_event(const TradeEvent& event);
void log_event(const OrderAcceptedEvent& event);

// Flush buffered writes
void flush();

// Load log for replay
std::vector<LogEntry> load_log(const std::string& path);
```

**Buffering Strategy**:
- Write to memory buffer (4KB typical)
- Flush to disk periodically or on demand
- Trade-off: Lower I/O overhead vs. data loss risk

**File Rotation**:
- New log file per trading day
- Format: `events_YYYYMMDD.log`
- Compress old logs (gzip)

### Timestamps

**Two Modes**:

1. **Wall Clock** (default):
   - `std::chrono::steady_clock::now()`
   - Real nanoseconds since epoch
   - Used for live trading

2. **Logical Clock** (deterministic):
   - Monotonic counter
   - Incremented on each event
   - Used for replay and testing

**Trade-off**: Wall clock provides real latency measurements; logical clock provides perfect determinism.

### Sequence Numbers

**Purpose**: Detect lost events, ensure ordering.

**Implementation**: Simple counter, incremented on each logged event.

**Gap Detection**: 
```cpp
if (event.seq != last_seq + 1) {
    log_error("Sequence gap detected!");
}
```

## Use Cases

### 1. Debugging Production Issues

**Scenario**: Customer reports unexpected fill price.

**Process**:
```bash
# Find relevant time range
grep "order_id:42" logs/events_20251127.log > debug.log

# Replay with validation
./replay --input debug.log --validate --print-trades

# Step through with debugger
gdb ./replay
(gdb) break OrderBook::match_order
(gdb) run --input debug.log
```

**Result**: Exact reproduction of production behavior, can inspect all state.

### 2. Backtesting Strategy

**Scenario**: Test new order placement strategy.

**Process**:
```python
# Load historical event log
events = load_events("logs/events_20251126.log")

# Inject synthetic orders from strategy
strategy_orders = my_strategy.generate_orders(events)

# Replay with strategy orders interleaved
replay_with_strategy(events, strategy_orders)

# Analyze PnL
analyze_results(trades, positions)
```

### 3. Compliance & Audit

**Scenario**: Regulator asks "Why did you trade at that price?"

**Process**:
```bash
# Extract all trades for symbol
./replay --input logs/events_20251127.log \
         --symbol AAPL \
         --print-trades \
         --stats > audit_report.txt

# Verify no wash trades
./replay --input logs/events_20251127.log \
         --check-self-trades \
         --fail-on-error
```

**Result**: Complete audit trail with provable correctness.

### 4. Fuzz Testing

**Scenario**: Find edge case bugs.

**Process**:
```cpp
// Generate 1M random orders
auto orders = generate_random_orders(1000000);

// Log to file
EventLog log("fuzz_test.log", true);
for (auto& order : orders) {
    engine.handle(order);
}
log.flush();

// Replay and validate
auto optimized_trades = replay_optimized("fuzz_test.log");
auto reference_trades = replay_reference("fuzz_test.log");

assert(optimized_trades == reference_trades);
```

## Comparison to Real Exchanges

### CME (Chicago Mercantile Exchange)

**Audit Trail System**:
- Every order/cancel/fill logged
- Microsecond timestamps
- Retained for 7 years
- Used for market surveillance

### NASDAQ

**ITCH Feed**:
- Complete market transparency
- Every order add/cancel/execute broadcast
- Can reconstruct entire book from ITCH feed
- Public (with subscription)

### Jane Street / Citadel (from interviews)

**Internal Replay Systems**:
- Every trading decision logged
- Replay with different parameters
- Used for strategy research
- Used for post-mortems

## Performance Considerations

### Overhead

**Logging Cost**:
- JSON serialization: ~500ns per event
- Buffered write: ~100ns amortized
- Flush to disk: ~1ms per batch
- **Total**: ~600ns per event in deterministic mode

**Trade-off**: 0.6μs overhead vs. complete auditability.

**Optimization**:
- Async I/O thread (lock-free queue)
- Binary format instead of JSON (3x faster)
- Memory-mapped files (zero-copy)

### Storage

**Typical Day**:
- 10M orders
- 5M trades
- ~500 bytes per event (JSON)
- **Total**: ~7.5 GB per day uncompressed

**Compression**: gzip reduces to ~1 GB per day.

**Retention**: Keep 30 days live, archive rest to S3/GCS.

## Best Practices

1. **Always Enable in Production**: Cost is minimal, value is enormous.
2. **Test Replay Regularly**: Verify logs are actually replayable.
3. **Compress Old Logs**: Save storage costs.
4. **Monitor Sequence Gaps**: Alert on missing events.
5. **Use Logical Clocks in Tests**: Perfect determinism.
6. **Snapshot Periodically**: Faster replay from checkpoint.
7. **Separate Log Per Symbol**: Easier analysis, parallel replay.

## Future Enhancements

1. **Binary Format**: Protocol Buffers or Cap'n Proto for speed
2. **Incremental Snapshots**: Only log deltas, faster replay
3. **Distributed Replay**: Parallel replay across machines
4. **Time Travel Debugging**: Rewind to any point in history
5. **Live Replay**: Replay production logs in test environment
6. **Automatic Regression Tests**: Replay daily logs against new code

## Summary

Determinism is not optional for serious trading systems. It provides:
- **Regulatory compliance** (provable audit trail)
- **Debugging superpower** (exact reproduction of bugs)
- **Research velocity** (fast iteration on strategies)
- **Legal protection** (explainable behavior)

The cost (sub-microsecond overhead) is negligible compared to the value. Every professional HFT/exchange system has deterministic replay—now you do too.

