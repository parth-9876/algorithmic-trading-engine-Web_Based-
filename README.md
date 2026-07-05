# AlgoExchange 📈

**AlgoExchange** is a high-performance exchange simulator and market analytics engine. It features a blazing-fast C++17 core for order matching and data crunching, paired with a sleek, modern, web-based fintech dashboard built in Python and Vanilla HTML/CSS/JS.

This project was built from scratch to showcase clean architecture, optimal time complexities, robust edge-case handling, and a premium user experience.

---

## 🌐 Web Dashboard

AlgoExchange features a beautiful, responsive web interface that communicates with the native C++ engine via a lightweight Python bridge.

### Features
- **Premium Fintech Aesthetic**: Dark mode, glassmorphism, and smooth animations.
- **Real-time Order Management**: Place Limit/Market orders with intuitive forms.
- **Actionable Analytics**: Explore price histories, calculate optimal order execution, and find market dependencies with a user-friendly UI.
- **Interactive Graphs**: Visualize price histories with smooth, anti-aliased Canvas charts that automatically downsample large datasets (e.g., thousands of trades into a clean 100-point moving average).
- **Terminal Emulator**: A built-in terminal to interact directly with the C++ engine from the browser.
- **Instant Engine Reset**: Hard reset the backend engine right from the settings tab.

### Running the Web Dashboard
1. Ensure the pre-compiled `AlgoExchange.exe` is in the `executable/` directory.
2. Start the Python server:
   ```powershell
   python server.py
   ```
3. Open your browser and go to `http://localhost:8000`.

---

## 🏗️ Architecture & Features (C++ Core)

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

## 🚀 Running the Pre-compiled Executable (Windows CLI)

If you prefer the command line, you can run the engine directly without the web dashboard.

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

## 🎮 CLI Usage Guide

If using the engine via the command line (or the Web Dashboard's Terminal), try these commands:

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

To run the automated test suite natively, pipe the provided test file into the executable. Using the pre-compiled version:
```powershell
Get-Content executable\test_comprehensive.txt | .\executable\AlgoExchange.exe
```
*(If you built from source, use `Get-Content data\test_comprehensive.txt | .\build\Release\AlgoExchange.exe`)*
