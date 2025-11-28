#!/usr/bin/env python3
"""
Generate sample market data CSV files for testing the LOB engine
"""

import argparse
import random
import csv
from datetime import datetime, timedelta

def generate_realistic_orders(symbols, num_orders, output_file):
    """Generate realistic order flow CSV"""
    
    # Starting mid prices for each symbol
    mid_prices = {symbol: random.uniform(100, 500) for symbol in symbols}
    
    orders = []
    current_time = datetime.now()
    order_id = 1
    active_orders = []
    
    for i in range(num_orders):
        symbol = random.choice(symbols)
        mid = mid_prices[symbol]
        
        # 60% new orders, 30% cancels, 10% replaces
        action_roll = random.random()
        
        if action_roll < 0.6 or len(active_orders) < 10:
            # New order
            action = "NEW"
            side = random.choice(["BUY", "SELL"])
            
            # Price relative to mid
            if side == "BUY":
                price = mid - random.uniform(0.01, 2.0)
            else:
                price = mid + random.uniform(0.01, 2.0)
            
            price = round(price, 2)
            quantity = random.randint(10, 1000)
            order_type = random.choice(["LIMIT"] * 8 + ["MARKET"] * 2)  # 80% LIMIT
            
            orders.append([
                current_time.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
                action,
                order_id,
                symbol,
                side,
                price,
                quantity,
                order_type
            ])
            
            active_orders.append(order_id)
            order_id += 1
            
        elif action_roll < 0.9 and active_orders:
            # Cancel
            action = "CANCEL"
            cancel_id = random.choice(active_orders)
            active_orders.remove(cancel_id)
            
            orders.append([
                current_time.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
                action,
                cancel_id,
                symbol,
                "",
                "",
                "",
                ""
            ])
            
        elif active_orders:
            # Replace
            action = "REPLACE"
            replace_id = random.choice(active_orders)
            side = random.choice(["BUY", "SELL"])
            
            if side == "BUY":
                price = mid - random.uniform(0.01, 2.0)
            else:
                price = mid + random.uniform(0.01, 2.0)
            
            price = round(price, 2)
            quantity = random.randint(10, 1000)
            
            orders.append([
                current_time.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
                action,
                replace_id,
                symbol,
                side,
                price,
                quantity,
                "LIMIT"
            ])
        
        # Drift mid price slightly
        if i % 100 == 0:
            mid_prices[symbol] += random.uniform(-0.5, 0.5)
        
        # Advance time by 1-10 milliseconds
        current_time += timedelta(milliseconds=random.randint(1, 10))
    
    # Write to CSV
    with open(output_file, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['timestamp', 'action', 'order_id', 'symbol', 
                        'side', 'price', 'quantity', 'order_type'])
        writer.writerows(orders)
    
    print(f"Generated {len(orders)} orders to {output_file}")
    print(f"Symbols: {', '.join(symbols)}")
    print(f"Active orders at end: {len(active_orders)}")


def generate_aggressive_cross(symbols, output_file):
    """Generate orders that will cross and generate trades"""
    
    orders = []
    current_time = datetime.now()
    order_id = 1
    
    for symbol in symbols:
        mid = 150.0
        
        # Post resting orders
        for i in range(5):
            # Buy side
            orders.append([
                current_time.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
                "NEW",
                order_id,
                symbol,
                "BUY",
                round(mid - (i + 1) * 0.01, 2),
                100 * (i + 1),
                "LIMIT"
            ])
            order_id += 1
            current_time += timedelta(milliseconds=1)
            
            # Sell side
            orders.append([
                current_time.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
                "NEW",
                order_id,
                symbol,
                "SELL",
                round(mid + (i + 1) * 0.01, 2),
                100 * (i + 1),
                "LIMIT"
            ])
            order_id += 1
            current_time += timedelta(milliseconds=1)
        
        # Send aggressive orders that cross
        orders.append([
            current_time.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
            "NEW",
            order_id,
            symbol,
            "BUY",
            round(mid + 0.03, 2),  # Cross 3 levels
            250,
            "LIMIT"
        ])
        order_id += 1
        current_time += timedelta(milliseconds=10)
        
        orders.append([
            current_time.strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
            "NEW",
            order_id,
            symbol,
            "SELL",
            round(mid - 0.03, 2),  # Cross 3 levels
            250,
            "LIMIT"
        ])
        order_id += 1
        current_time += timedelta(milliseconds=10)
    
    with open(output_file, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['timestamp', 'action', 'order_id', 'symbol', 
                        'side', 'price', 'quantity', 'order_type'])
        writer.writerows(orders)
    
    print(f"Generated aggressive crossing orders to {output_file}")


def main():
    parser = argparse.ArgumentParser(description='Generate sample market data')
    parser.add_argument('--output', default='sample_orders.csv', 
                       help='Output CSV file')
    parser.add_argument('--symbols', default='AAPL,MSFT,GOOGL',
                       help='Comma-separated symbols')
    parser.add_argument('--count', type=int, default=1000,
                       help='Number of orders to generate')
    parser.add_argument('--type', choices=['realistic', 'aggressive'], 
                       default='realistic',
                       help='Type of data to generate')
    
    args = parser.parse_args()
    
    symbols = args.symbols.split(',')
    
    print(f"Generating {args.type} market data...")
    
    if args.type == 'realistic':
        generate_realistic_orders(symbols, args.count, args.output)
    else:
        generate_aggressive_cross(symbols, args.output)
    
    print(f"\nTo replay:")
    print(f"  ./build/csv_replay {args.output}")


if __name__ == '__main__':
    main()

