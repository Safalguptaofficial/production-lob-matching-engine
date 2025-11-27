# Market Microstructure Fundamentals

## What is a Limit Order Book?

A **Limit Order Book** (LOB) is the fundamental data structure used by modern electronic exchanges to match buyers and sellers. It maintains all active orders for a security, organized by price and time.

### Basic Concepts

**Order**: An instruction to buy or sell a security at a specified (or market) price.

**Bid**: An order to buy. "I want to buy 100 shares at $150.00"

**Ask** (or Offer): An order to sell. "I want to sell 50 shares at $150.50"

**Spread**: The difference between the best (highest) bid and the best (lowest) ask.
- Tight spread (small difference): liquid market, easy to trade
- Wide spread (large difference): illiquid market, higher trading cost

**Mid-Price**: The average of best bid and best ask: `(best_bid + best_ask) / 2`

**Depth**: The total quantity available at each price level, or the number of price levels with active orders.

**Liquidity**: The ease with which an asset can be bought or sold. High liquidity = tight spreads, large depth.

## Example Order Book

```
         ASKS (sellers)                  
Price    | Quantity | Orders             
─────────┼──────────┼────────            
 $150.55 |     200  |   [O8]             
 $150.52 |     500  | [O6, O7]  ← Best Ask (lowest price to buy at)
 $150.51 |     300  |   [O5]             
─────────┴──────────┴────────            
              ↕                          
           SPREAD = $0.01                
              ↕                          
─────────┬──────────┬────────            
 $150.50 |     400  | [O1, O2]  ← Best Bid (highest price to sell at)
 $150.49 |     600  | [O3, O4]           
 $150.48 |     100  |   [O9]             
         BIDS (buyers)                   
```

**Best Bid**: $150.50 (highest price someone is willing to pay)
**Best Ask**: $150.51 (lowest price someone is willing to sell)
**Spread**: $0.01
**Mid-Price**: $150.505

## Price-Time Priority

Most exchanges use **price-time priority** matching:

### Price Priority
- Buy orders: Higher price has priority
- Sell orders: Lower price has priority
- Best bid/ask are always at the top

### Time Priority (FIFO)
- At the same price level, orders are matched in the order they arrived
- Earlier orders get filled first
- This is why "queue position" matters to market makers

### Example

If three buy orders arrive at $150.50:
```
Time 10:00:01 → Order A: Buy 100 @ $150.50
Time 10:00:02 → Order B: Buy 200 @ $150.50
Time 10:00:03 → Order C: Buy 150 @ $150.50

Queue at $150.50: [A, B, C]  ← FIFO order
```

When a sell order comes in at $150.50:
```
Sell 250 @ $150.50
  → Trades:
     - 100 with Order A (fully fills A)
     - 150 with Order B (partially fills B, 50 remaining)
  → Queue at $150.50: [B (50 qty), C]
```

## Order Types

### Limit Order
- Specifies a price and quantity
- Will only execute at the limit price or better
- **Better** = higher price for sells, lower price for buys
- If not immediately executable, rests on the book

**Example**: 
```
Buy 100 @ $150.50 (limit)
  → Will execute at $150.50 or lower
  → If best ask is $150.52, order goes on bid side
  → If best ask is $150.48, order executes at $150.48
```

### Market Order
- No price specified, executes at best available price
- Guarantees execution (if liquidity exists)
- **Does not guarantee price**
- Takes liquidity from the book

**Example**:
```
Market Buy 300 shares
  → Matches against best asks until filled:
    - 100 @ $150.51
    - 200 @ $150.52
  → Average fill price: $150.5167
```

### IOC (Immediate-Or-Cancel)
- Executes immediately against available liquidity
- Any unfilled portion is cancelled
- Never rests on the book

**Example**:
```
IOC Buy 500 @ $150.52
  Available liquidity: 300 @ $150.51, 200 @ $150.52
  → Fills 500 shares
  
IOC Buy 500 @ $150.51
  Available liquidity: 300 @ $150.51
  → Fills 300 shares, cancels remaining 200
```

### FOK (Fill-Or-Kill)
- All-or-nothing execution
- Either fills entire order immediately or rejects
- Never rests on the book

**Example**:
```
FOK Buy 500 @ $150.52
  Available liquidity: 300 @ $150.51, 200 @ $150.52
  → Fills entire 500 shares ✓
  
FOK Buy 600 @ $150.52
  Available liquidity: 300 @ $150.51, 200 @ $150.52 (total: 500)
  → Rejects entire order (insufficient liquidity) ✗
```

## Why Does This Matter in HFT?

### 1. Queue Position is Alpha
- Being first in line at a price level gives you priority
- Market makers race to cancel and update quotes to stay at the front
- Milliseconds matter (actually, microseconds)

### 2. Spread Capture
- Market makers profit from the spread
- Post limit orders on both sides:
  - Bid: $150.50
  - Ask: $150.51
  - Profit: $0.01 per round-trip (if both fill)

### 3. Adverse Selection
- When you're at the front of the queue, you're most exposed to **informed traders**
- If price is about to move, informed traders hit your quote first
- Managing adverse selection is key to market making profitability

### 4. Latency Arbitrage
- Faster traders can:
  - See price movements on other venues first
  - Cancel stale quotes before they're hit
  - Capture spread before slower participants

### 5. Order Flow Toxicity
- Not all order flow is equal
- **Toxic flow**: Informed traders, likely to move against you
- **Non-toxic flow**: Retail, likely to mean-revert
- HFT firms spend millions detecting and avoiding toxic flow

## Comparison to Real Exchanges

### CME (Chicago Mercantile Exchange)
- Futures market
- Uses price-time priority
- **MDP 3.0** (Market Data Protocol): Binary market data feed
- **iLink 3**: Order entry protocol

### NASDAQ
- Equities market
- Price-time priority for displayed orders
- **ITCH**: Market data feed showing every order add/cancel/execute
- **OUCH**: Order entry protocol
- Hidden orders exist but have lower priority

### NYSE
- Hybrid market (electronic + floor)
- **Designated Market Makers** (DMMs) have special obligations
- **ARCA**: Fully electronic venue, price-time priority

### Binance (Crypto)
- Fully electronic
- Price-time priority
- WebSocket market data feeds
- REST + WebSocket order entry

## How This Engine Compares

Our implementation mimics core exchange behavior:
- ✅ Price-time priority (like all major exchanges)
- ✅ Multi-level order book (like NASDAQ depth-of-book)
- ✅ IOC/FOK support (like CME, NASDAQ)
- ✅ Deterministic replay (like regulatory audit logs)
- ✅ Binary market data feeds (like CME MDP3, NASDAQ ITCH)
- ✅ Self-trade prevention (exchange requirement)

**Missing** (intentionally, for simplicity):
- ❌ Multiple venues / routing
- ❌ Auction mechanisms (opening/closing)
- ❌ Market maker incentives (rebates)
- ❌ Iceberg / hidden orders (can be added)
- ❌ Networking / FIX protocol (can be added)

## Further Reading

**Books**:
- *Flash Boys* by Michael Lewis (accessible introduction to HFT)
- *Trading and Exchanges* by Larry Harris (academic treatment)
- *Algorithmic Trading* by Barry Johnson (practical guide)

**Papers**:
- "High-Frequency Trading and Market Microstructure" (Easley, Lopez de Prado, O'Hara)
- "The Flash Crash" (Kirilenko et al., SEC report)
- "Measuring Adverse Selection in Order Flow" (Glosten & Harris)

**Exchange Documentation**:
- CME MDP 3.0 Specification
- NASDAQ TotalView-ITCH Protocol
- NYSE Pillar Market Data Specification

## Summary

Understanding limit order books and microstructure is essential for:
- **Traders**: Optimizing execution, minimizing market impact
- **Market Makers**: Capturing spread, managing inventory and adverse selection
- **Engineers**: Building fast, correct matching engines and risk systems
- **Quantitative Researchers**: Modeling price formation and information diffusion

This engine provides a foundation for exploring these concepts in a production-like environment.

