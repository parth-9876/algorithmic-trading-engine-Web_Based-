# AlgoExchange 📈

**AlgoExchange** is a high-performance, command-line exchange simulator and market analytics engine built in C++17. It demonstrates the application of advanced data structures and algorithmic design to solve real-world problems in financial technology, without relying on external libraries (pure STL).

This project was built from scratch to showcase clean architecture, optimal time complexities, and robust edge-case handling.

---

## 🏗️ Architecture & Features

AlgoExchange is divided into several independent modules, each leveraging specific data structures to achieve optimal performance:

### 1. Matching Engine (Order Book)
The core exchange matches limit and market orders using **Price-Time Priority**.
- **Data Structures**: `std::unordered_map` coupled with `std::set` and custom comparators.
- **Performance**:
  - Insert / Match / Cancel / Modify: **O(log N)**
  - Lookups: **O(1)**
- *Handles partial fills, resting market orders, and latency profiling at the microsecond level.*

### 2. Autocomplete Engine
Rapid symbol lookup (e.g., typing "GO" returns "GOOG" and "GOOGL").
- **Data Structure**: Prefix **Trie**.
- **Performance**:
  - Insert / Search: **O(L)** where L is symbol length.
  - Autocomplete: **O(L + K)** where K is number of matches.

### 3. Historical Price Analytics
Fast range queries over historical price data (MAX, MIN, SUM, AVG, VOLUME).
- **Data Structure**: Array-backed **Segment Tree**.
- **Performance**:
  - Build: **O(N)**
  - Point Update: **O(log N)**
  - Range Query: **O(log N)**

### 4. Market Dashboards
Real-time tracking of Top Gainers, Top Losers, Highest Volume, and Most Active stocks.
- **Data Structure**: Min/Max **Heaps** (`std::priority_queue`).
- **Performance**: **O(N log N)** building the top K elements on demand.

### 5. Market Relationships
Tracks supplier and correlation dependencies between tickers to find shortest paths.
- **Algorithm**: **Dijkstra’s Algorithm** over an Adjacency List.
- **Performance**: **O((V + E) log V)**.

### 6. Execution Optimizer
Finds the cheapest way to break down a massive block order (e.g. 10,000 shares) into standard lot sizes, balancing fixed trade fees vs. quadratic market impact costs.
- **Algorithm**: **Dynamic Programming** (Unbounded Knapsack variant).
- **Performance**: **O(QTY * |Lot Sizes|)**.

---

## 🚀 Running the Pre-compiled Executable (Windows)

The project includes a pre-compiled executable so you don't have to build it from scratch.

### Instructions

1. Navigate to the `executable/` folder.
2. Run `AlgoExchange.exe`.

```powershell
cd executable
.\AlgoExchange.exe
```

*Note: The `executable/` folder already includes the necessary `data/` files (like `stocks.txt`, `prices.txt`, etc.) for the engine to run out of the box.*

---

## 🛠️ Building from Source (Optional)

If you prefer to compile the engine yourself, the project uses CMake and is designed to build on Windows with MSVC.

### Prerequisites
- CMake (3.10+)
- Visual Studio (with Desktop development with C++ workload)

### Compilation
Open a **Developer PowerShell for VS** and run:

```powershell
# 1. Create a build directory
mkdir build
cd build

# 2. Generate build files
cmake ..

# 3. Compile in Release mode
cmake --build . --config Release

# 4. Run the newly compiled executable
cd ..
.\build\Release\AlgoExchange.exe
```

---

## 🎮 Usage Guide

Once inside the interactive REPL (`AlgoExchange>`), try the following commands:

**Order Management**
```text
BUY AAPL 100 190.00          # Place limit buy
SELL AAPL 50 189.50          # Place limit sell
MARKET BUY TSLA 50           # Place market buy
CANCEL 1                     # Cancel order by ID
MODIFY 2 BUY 100 200.00      # Modify an existing order
```

**Market Views**
```text
SHOW_ORDERBOOK AAPL          # View the orderbook for AAPL
SHOW_TRADES                  # View trade history log
SEARCH_SYMBOL MS             # Find stocks starting with 'MS'
```

**Analytics & Testing**
```text
MARKET_SUMMARY               # View total volume, gainers, losers
PRICE_HISTORY MSFT           # See historical prices
RANGE_QUERY MSFT MAX 0 5     # Query max price in a specific timeframe
DEPENDENCY_PATH AAPL NVDA    # Find supplier/correlation path
OPTIMIZE_ORDER BUY AAPL 5000 # DP calculation for order splitting
STRESS_TEST 10000            # Run native C++ in-memory speed test
```

**General**
```text
HELP                         # Show available commands
EXIT                         # Close the program
DROP_EXIT                    # Wipe generated stress test logs and close
```

## 🧪 Testing

The engine is resilient against invalid inputs, handles extreme edge cases (like market orders crossing market orders), and avoids floating-point exceptions. 

To run the automated test suite, pipe the provided test file into the executable. Using the pre-compiled version:
```powershell
Get-Content executable\test_comprehensive.txt | .\executable\AlgoExchange.exe
```
*(If you built from source, use `Get-Content data\test_comprehensive.txt | .\build\Release\AlgoExchange.exe`)*
