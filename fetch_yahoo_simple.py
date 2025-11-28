#!/usr/bin/env python3
"""Simple Yahoo Finance data fetcher - no imports needed if yfinance fails"""
import sys
import csv
from datetime import datetime

# Try to import yfinance
try:
    import yfinance as yf
    YFINANCE_AVAILABLE = True
except ImportError:
    YFINANCE_AVAILABLE = False
    print("⚠️  yfinance not available in this Python environment")
    print("    Installed Python:", sys.executable)
    print()
    print("💡 Try one of these:")
    print("    1. Use conda's python: ~/opt/anaconda3/bin/python3 fetch_yahoo_simple.py AAPL")
    print("    2. Or: conda activate base && python3 fetch_yahoo_simple.py AAPL")
    sys.exit(1)

def fetch_yahoo_data(symbol, output_file):
    """Fetch real intraday data from Yahoo Finance"""
    print(f"📡 Fetching real market data for {symbol} from Yahoo Finance...")
    
    try:
        ticker = yf.Ticker(symbol)
        hist = ticker.history(period="1d", interval="1m")
        
        if hist.empty:
            print(f"❌ No data found for {symbol}")
            print("   (Markets may be closed)")
            return False
        
        print(f"✅ Fetched {len(hist)} data points")
        
        with open(output_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'symbol', 'side', 'order_type', 'price', 'quantity', 'order_id', 'trader_id'])
            
            order_id = 1
            for timestamp, row in hist.iterrows():
                ts = int(timestamp.timestamp() * 1000)
                
                # Create buy orders at low price
                writer.writerow([ts, symbol, 'BUY', 'LIMIT', row['Low'], int(row['Volume']//100), order_id, 1000])
                order_id += 1
                
                # Create sell orders at high price
                writer.writerow([ts, symbol, 'SELL', 'LIMIT', row['High'], int(row['Volume']//100), order_id, 1001])
                order_id += 1
                
                # Add some market orders
                if order_id % 10 == 0:
                    writer.writerow([ts+500, symbol, 'BUY', 'MARKET', 0, int(row['Volume']//200), order_id, 1002])
                    order_id += 1
        
        print(f"✅ Saved to {output_file}")
        print(f"   Orders created: {order_id-1}")
        print(f"   Time range: {hist.index[0]} to {hist.index[-1]}")
        print(f"   Price range: ${hist['Low'].min():.2f} - ${hist['High'].max():.2f}")
        print()
        print(f"🚀 Ready to replay:")
        print(f"   ./build/csv_replay {output_file}")
        return True
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 fetch_yahoo_simple.py <SYMBOL> [output.csv]")
        print()
        print("Examples:")
        print("  python3 fetch_yahoo_simple.py AAPL")
        print("  python3 fetch_yahoo_simple.py TSLA tsla_real.csv")
        sys.exit(1)
    
    symbol = sys.argv[1].upper()
    output = sys.argv[2] if len(sys.argv) > 2 else f"{symbol.lower()}_real.csv"
    
    fetch_yahoo_data(symbol, output)

