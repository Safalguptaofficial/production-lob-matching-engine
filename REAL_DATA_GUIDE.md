# 📊 Using Real Market Data with LOB Engine

This guide shows you how to use **real market data** instead of synthetic/demo data.

---

## 🎯 Quick Start (CSV Data)

### 1. Generate Realistic Market Data

```bash
cd /Users/safalgupta/Desktop/lob

# Make script executable
chmod +x scripts/generate_real_data.py

# Generate 10,000 realistic orders for AAPL
python3 scripts/generate_real_data.py --symbol AAPL --orders 10000 --output market_data.csv
```

### 2. Build the CSV Replay Tool

```bash
cmake --build build --target csv_replay
```

### 3. Run with Real Data

```bash
./build/csv_replay market_data.csv
```

**Output:**
```
=== Real Market Data Replay ===
Loading orders from: market_data.csv

Registered symbol: AAPL
TRADE [AAPL] 100 @ $150.25
TRADE [AAPL] 200 @ $150.30
Progress: 1000 orders processed, 456 trades
...
```

---

## 📁 CSV Format

Your CSV file should have this format:

```csv
timestamp,symbol,side,order_type,price,quantity,order_id,trader_id
1638360000000,AAPL,BUY,LIMIT,150.25,100,1,1001
1638360001000,AAPL,SELL,LIMIT,150.26,50,2,1002
1638360002000,AAPL,BUY,MARKET,0,200,3,1003
```

**Fields:**
- `timestamp`: Unix timestamp in milliseconds
- `symbol`: Stock ticker (e.g., AAPL, MSFT, GOOGL)
- `side`: BUY or SELL
- `order_type`: LIMIT or MARKET
- `price`: Price in dollars (e.g., 150.25)
- `quantity`: Number of shares
- `order_id`: Unique order ID
- `trader_id`: Trader/participant ID

---

## 🌐 Getting Real Historical Data

### Option 1: Yahoo Finance (FREE - Most Popular)

```bash
# Install yfinance
pip3 install yfinance

# Fetch today's 1-minute data for AAPL
python3 scripts/generate_real_data.py --mode yahoo --symbol AAPL --output aapl_real.csv

# Replay it
./build/csv_replay aapl_real.csv
```

**What you get:**
- Real intraday prices
- Actual trading volumes
- Free, no API key needed

### Option 2: Polygon.io (FREE tier - Professional grade)

```bash
# Sign up at polygon.io for free API key
# Then:
python3 scripts/generate_real_data.py \
    --mode polygon \
    --symbol AAPL \
    --date 2024-11-27 \
    --api-key YOUR_API_KEY \
    --output polygon_data.csv

./build/csv_replay polygon_data.csv
```

**What you get:**
- Tick-by-tick data
- Multiple exchanges
- Professional quality

### Option 3: Download from Kaggle

```bash
# Go to https://www.kaggle.com/datasets
# Search: "stock market tick data" or "order book data"
# Popular datasets:
# - LOBSTER (Limit Order Book data)
# - TAQ (Trade and Quote)
# - NASDAQ ITCH messages

# Download and convert to our format
```

---

## 🔴 Live Real-Time Data

### Option 1: Coinbase Pro (Crypto - FREE)

**Install WebSocket library:**
```bash
brew install websocketpp
# or
pip3 install websocket-client
```

**Python WebSocket Bridge:**
```python
# scripts/coinbase_feed.py
import websocket
import json
import csv

def on_message(ws, message):
    data = json.loads(message)
    if data['type'] == 'match':
        # Write to CSV
        with open('live_data.csv', 'a') as f:
            writer = csv.writer(f)
            writer.writerow([
                int(data['time']),
                data['product_id'],
                data['side'].upper(),
                'MARKET',
                data['price'],
                data['size'],
                data['trade_id'],
                0
            ])

ws = websocket.WebSocketApp(
    "wss://ws-feed.pro.coinbase.com",
    on_message=on_message
)

# Subscribe
ws.on_open = lambda ws: ws.send(json.dumps({
    "type": "subscribe",
    "channels": [{"name": "matches", "product_ids": ["BTC-USD"]}]
}))

ws.run_forever()
```

**Run:**
```bash
# Terminal 1: Collect live data
python3 scripts/coinbase_feed.py

# Terminal 2: Replay in real-time
tail -f live_data.csv | while read line; do
    echo "$line" | ./build/csv_replay /dev/stdin
done
```

### Option 2: Binance (Crypto - FREE)

```bash
# WebSocket: wss://stream.binance.com:9443/ws/btcusdt@trade

# Similar approach as Coinbase
```

### Option 3: IEX Cloud (Stocks - FREE tier)

```bash
# Sign up at iexcloud.io
# Use their REST API to get real-time quotes
```

---

## 📈 Real Data Sources Comparison

| Source | Asset Type | Cost | Quality | Real-time | Best For |
|--------|-----------|------|---------|-----------|----------|
| **Yahoo Finance** | Stocks | FREE | Good | 15min delay | Learning, backtesting |
| **Polygon.io** | Stocks | FREE tier | Excellent | Yes (paid) | Professional dev |
| **Coinbase Pro** | Crypto | FREE | Excellent | Yes | Live crypto data |
| **Binance** | Crypto | FREE | Excellent | Yes | Live crypto data |
| **IEX Cloud** | Stocks | FREE tier | Good | Yes | Real-time stocks |
| **Alpha Vantage** | Stocks | FREE tier | Good | 15min delay | API integration |
| **Kaggle** | Various | FREE | Varies | No | Research/backtesting |
| **LOBSTER** | Stocks | Paid | Excellent | No | Academic research |

---

## 💻 Code Integration Examples

### Example 1: Process CSV in Your Code

```cpp
#include <lob/matching_engine.hpp>
#include <fstream>
#include <sstream>

void process_real_data(const std::string& csv_file) {
    lob::MatchingEngine engine;
    engine.add_symbol({"AAPL", 1, 1, 1});
    
    std::ifstream file(csv_file);
    std::string line;
    
    while (std::getline(file, line)) {
        // Parse CSV line
        std::stringstream ss(line);
        std::string timestamp, symbol, side, type, price, qty, order_id, trader_id;
        
        std::getline(ss, timestamp, ',');
        std::getline(ss, symbol, ',');
        std::getline(ss, side, ',');
        std::getline(ss, type, ',');
        std::getline(ss, price, ',');
        std::getline(ss, qty, ',');
        std::getline(ss, order_id, ',');
        std::getline(ss, trader_id, ',');
        
        // Submit to engine
        lob::NewOrderRequest request{
            .order_id = std::stoull(order_id),
            .trader_id = std::stoull(trader_id),
            .symbol = symbol,
            .side = side == "BUY" ? lob::Side::BUY : lob::Side::SELL,
            .order_type = type == "MARKET" ? lob::OrderType::MARKET : lob::OrderType::LIMIT,
            .price = static_cast<lob::Price>(std::stod(price) * 100),
            .quantity = std::stoull(qty),
            .time_in_force = lob::TimeInForce::DAY
        };
        
        auto response = engine.handle(request);
        
        // Process trades
        for (const auto& trade : response.trades) {
            std::cout << "Trade: " << trade.quantity 
                      << " @ $" << trade.price / 100.0 << "\n";
        }
    }
}
```

### Example 2: Real-Time WebSocket Handler

```cpp
#include <lob/matching_engine.hpp>

class RealtimeHandler {
    lob::MatchingEngine engine_;
    
public:
    RealtimeHandler() {
        engine_.add_symbol({"BTC-USD", 1, 1, 1});
    }
    
    void on_websocket_message(const std::string& json_msg) {
        // Parse JSON (use nlohmann::json)
        auto msg = nlohmann::json::parse(json_msg);
        
        if (msg["type"] == "trade") {
            lob::NewOrderRequest request{
                .order_id = msg["trade_id"],
                .trader_id = 1,
                .symbol = msg["symbol"],
                .side = msg["side"] == "buy" ? lob::Side::BUY : lob::Side::SELL,
                .order_type = lob::OrderType::MARKET,
                .price = static_cast<lob::Price>(msg["price"].get<double>() * 100),
                .quantity = static_cast<lob::Quantity>(msg["size"].get<double>() * 100),
                .time_in_force = lob::TimeInForce::IOC
            };
            
            engine_.handle(request);
        }
    }
};
```

---

## 📊 Real Data Workflow

### Complete Pipeline:

```bash
# 1. Get real data
python3 scripts/generate_real_data.py --mode yahoo --symbol AAPL --output data.csv

# 2. Replay and analyze
./build/csv_replay data.csv > results.txt

# 3. Extract insights
grep "TRADE" results.txt | wc -l  # Count trades
grep "Final Book States" results.txt -A 10  # See final prices

# 4. Use for backtesting
# Modify csv_replay.cpp to add your strategy logic
```

### Strategy Backtesting Example:

```cpp
// In csv_replay.cpp, add your strategy:
double position = 0;
double pnl = 0;

for (auto& trade : response.trades) {
    // Simple momentum strategy
    if (trade.aggressor_side == lob::Side::BUY) {
        // Buy on upward momentum
        position += trade.quantity;
        pnl -= trade.quantity * trade.price / 100.0;
    } else {
        // Sell on downward momentum  
        position -= trade.quantity;
        pnl += trade.quantity * trade.price / 100.0;
    }
}

std::cout << "Final PnL: $" << pnl << "\n";
std::cout << "Position: " << position << " shares\n";
```

---

## 🎯 Quick Commands

```bash
# Generate data
python3 scripts/generate_real_data.py --symbol AAPL --orders 50000 --output test.csv

# Replay
./build/csv_replay test.csv

# With statistics
./build/csv_replay test.csv 2>&1 | tee output.log

# Extract trades only
./build/csv_replay test.csv | grep "TRADE"

# Count by symbol
./build/csv_replay test.csv | grep "TRADE" | awk '{print $2}' | sort | uniq -c
```

---

## 🔧 Troubleshooting

### "Cannot open file"
```bash
# Check file exists
ls -lh market_data.csv

# Check format
head -3 market_data.csv
```

### "Error parsing line"
```bash
# Validate CSV format
python3 -c "import csv; f=open('data.csv'); csv.reader(f).next(); print('OK')"
```

### "Symbol not registered"
The engine auto-registers symbols in csv_replay tool!

---

## 🚀 Next Steps

1. **Get real data**: Use Yahoo Finance (easiest)
2. **Run csv_replay**: Process real market data
3. **Analyze results**: Look at trades, book states
4. **Add strategy**: Modify csv_replay.cpp with your logic
5. **Backtest**: Measure performance on real data

---

## 📚 Resources

- **Free Data Sources:**
  - Yahoo Finance: https://finance.yahoo.com
  - Polygon.io: https://polygon.io (free tier)
  - IEX Cloud: https://iexcloud.io (free tier)
  - Kaggle Datasets: https://www.kaggle.com/datasets

- **WebSocket Examples:**
  - Coinbase Pro: https://docs.pro.coinbase.com/#websocket-feed
  - Binance: https://binance-docs.github.io/apidocs/spot/en/#websocket-market-streams

- **Historical Data:**
  - LOBSTER: https://lobsterdata.com
  - NYSE TAQ: https://www.nyse.com/market-data/historical

---

**🎉 You're now ready to use REAL market data with your LOB engine!**

Start with: `python3 scripts/generate_real_data.py --mode yahoo --symbol AAPL`
