#!/usr/bin/env python3
"""
Generate realistic market data CSV for testing
Can also fetch real historical data from various sources
"""

import argparse
import random
import csv
from datetime import datetime, timedelta

def generate_realistic_orders(symbol, num_orders, output_file):
    """Generate realistic order flow"""
    print(f"Generating {num_orders} realistic orders for {symbol}...")
    
    with open(output_file, 'w', newline='') as f:
        writer = csv.writer(f)
        # Write header
        writer.writerow(['timestamp', 'symbol', 'side', 'order_type', 'price', 'quantity', 'order_id', 'trader_id'])
        
        # Starting conditions
        mid_price = 150.00
        timestamp = int(datetime.now().timestamp() * 1000)
        
        for i in range(num_orders):
            # Random walk for mid price
            mid_price += random.uniform(-0.10, 0.10)
            mid_price = max(100.0, min(200.0, mid_price))  # Keep in range
            
            # 80% limit orders, 20% market orders
            order_type = 'LIMIT' if random.random() < 0.8 else 'MARKET'
            
            # 50-50 buy/sell
            side = 'BUY' if random.random() < 0.5 else 'SELL'
            
            # Price around mid with spread
            if order_type == 'LIMIT':
                spread = random.uniform(0.01, 0.10)
                if side == 'BUY':
                    price = round(mid_price - spread, 2)
                else:
                    price = round(mid_price + spread, 2)
            else:
                price = 0.0  # Market order
            
            # Realistic quantities (round lots)
            quantity = random.choice([100, 200, 500, 1000, 2000, 5000])
            
            # Unique IDs
            order_id = i + 1
            trader_id = random.randint(1000, 1100)
            
            # Timestamp increments (1-100ms between orders)
            timestamp += random.randint(1, 100)
            
            writer.writerow([
                timestamp,
                symbol,
                side,
                order_type,
                price,
                quantity,
                order_id,
                trader_id
            ])
    
    print(f"✅ Generated {output_file}")
    print(f"   Symbol: {symbol}")
    print(f"   Orders: {num_orders}")
    print(f"   Format: CSV with header")

def fetch_polygon_data(symbol, date, api_key, output_file):
    """Fetch real tick data from Polygon.io"""
    try:
        import requests
        
        print(f"Fetching real data from Polygon.io for {symbol} on {date}...")
        
        url = f"https://api.polygon.io/v2/aggs/ticker/{symbol}/range/1/minute/{date}/{date}"
        params = {'apiKey': api_key}
        
        response = requests.get(url, params=params)
        data = response.json()
        
        if 'results' not in data:
            print("❌ No data found. Check symbol and date.")
            return
        
        print(f"✅ Fetched {len(data['results'])} data points")
        
        with open(output_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'symbol', 'side', 'order_type', 'price', 'quantity', 'order_id', 'trader_id'])
            
            order_id = 1
            for bar in data['results']:
                # Convert OHLCV to synthetic orders
                timestamp = bar['t']
                
                # Create bid at low, ask at high
                writer.writerow([timestamp, symbol, 'BUY', 'LIMIT', bar['l'], bar['v']//4, order_id, 1000])
                order_id += 1
                writer.writerow([timestamp, symbol, 'SELL', 'LIMIT', bar['h'], bar['v']//4, order_id, 1001])
                order_id += 1
        
        print(f"✅ Saved to {output_file}")
        
    except ImportError:
        print("❌ requests library not found. Install: pip install requests")
    except Exception as e:
        print(f"❌ Error: {e}")

def fetch_yahoo_data(symbol, output_file):
    """Fetch real historical data from Yahoo Finance"""
    try:
        import yfinance as yf
        
        print(f"Fetching real data from Yahoo Finance for {symbol}...")
        
        ticker = yf.Ticker(symbol)
        hist = ticker.history(period="1d", interval="1m")
        
        if hist.empty:
            print("❌ No data found")
            return
        
        print(f"✅ Fetched {len(hist)} data points")
        
        with open(output_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'symbol', 'side', 'order_type', 'price', 'quantity', 'order_id', 'trader_id'])
            
            order_id = 1
            for timestamp, row in hist.iterrows():
                ts = int(timestamp.timestamp() * 1000)
                
                # Create orders from OHLC
                writer.writerow([ts, symbol, 'BUY', 'LIMIT', row['Low'], int(row['Volume']//100), order_id, 1000])
                order_id += 1
                writer.writerow([ts, symbol, 'SELL', 'LIMIT', row['High'], int(row['Volume']//100), order_id, 1001])
                order_id += 1
        
        print(f"✅ Saved to {output_file}")
        
    except ImportError:
        print("❌ yfinance library not found. Install: pip install yfinance")
    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    parser = argparse.ArgumentParser(description='Generate or fetch real market data')
    parser.add_argument('--mode', choices=['generate', 'polygon', 'yahoo'], default='generate',
                       help='Data source mode')
    parser.add_argument('--symbol', default='AAPL', help='Stock symbol')
    parser.add_argument('--orders', type=int, default=10000, help='Number of orders (generate mode)')
    parser.add_argument('--output', default='market_data.csv', help='Output CSV file')
    parser.add_argument('--api-key', help='Polygon.io API key (polygon mode)')
    parser.add_argument('--date', help='Date for Polygon data (YYYY-MM-DD)')
    
    args = parser.parse_args()
    
    if args.mode == 'generate':
        generate_realistic_orders(args.symbol, args.orders, args.output)
    elif args.mode == 'polygon':
        if not args.api_key:
            print("❌ --api-key required for Polygon mode")
            return
        fetch_polygon_data(args.symbol, args.date or datetime.now().strftime('%Y-%m-%d'), 
                          args.api_key, args.output)
    elif args.mode == 'yahoo':
        fetch_yahoo_data(args.symbol, args.output)
    
    print(f"\n📊 Ready to replay:")
    print(f"   ./build/csv_replay {args.output}")

if __name__ == '__main__':
    main()

