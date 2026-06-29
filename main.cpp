// ============================================================================
//  main.cpp — AlgoExchange Entry Point & Command Dispatcher
// ============================================================================
//
//  This file #includes all other .cpp files to form a single translation unit.
//  CMake only compiles this file.
//
// ============================================================================

#include "utils.cpp"
#include "exchange.cpp"
#include "algorithms.cpp"
#include "analytics.cpp"

#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
//  Default stock symbols (pre-loaded into the Trie)
// ---------------------------------------------------------------------------

const std::vector<std::string> DEFAULT_SYMBOLS = {
    "AAPL", "AMZN", "MSFT", "GOOG", "GOOGL", "META", "TSLA", "NVDA",
    "NFLX", "ADBE", "CRM", "ORCL", "INTC", "AMD", "QCOM", "CSCO",
    "IBM", "UBER", "LYFT", "SNAP", "SQ", "PYPL", "V", "MA",
    "JPM", "GS", "BAC", "WFC", "C", "DIS", "CMCSA", "T", "VZ",
    "BA", "LMT", "GE", "CAT", "JNJ", "PFE", "UNH", "MRNA",
    "KO", "PEP", "MCD", "SBUX", "WMT", "TGT", "COST", "HD", "LOW"
};

// ---------------------------------------------------------------------------
//  Help text
// ---------------------------------------------------------------------------

void printBanner() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════╗\n";
    std::cout << "  ║          AlgoExchange v1.0                          ║\n";
    std::cout << "  ║   High Performance Exchange & Market Analytics      ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void printHelp() {
    std::cout << "\n  Available Commands:\n\n";
    std::cout << "  Order Management:\n";
    std::cout << "    BUY <symbol> <qty> <price>           Place a limit buy order\n";
    std::cout << "    SELL <symbol> <qty> <price>          Place a limit sell order\n";
    std::cout << "    MARKET BUY <symbol> <qty>            Place a market buy order\n";
    std::cout << "    MARKET SELL <symbol> <qty>           Place a market sell order\n";
    std::cout << "    CANCEL <orderID>                     Cancel an order\n";
    std::cout << "    MODIFY <orderID> <side> <qty> <price>  Modify an order\n";
    std::cout << "\n";
    std::cout << "  Display:\n";
    std::cout << "    SHOW_ORDERBOOK [symbol]              Show order book\n";
    std::cout << "    SHOW_TRADES                          Show trade history\n";
    std::cout << "    PERFORMANCE                          Show performance stats\n";
    std::cout << "\n";
    std::cout << "  Search:\n";
    std::cout << "    SEARCH_SYMBOL <prefix>               Autocomplete stock symbol\n";
    std::cout << "\n";
    std::cout << "  Analytics:\n";
    std::cout << "    TOP_GAINERS                          Top gaining stocks\n";
    std::cout << "    TOP_LOSERS                           Top losing stocks\n";
    std::cout << "    PRICE_HISTORY <symbol>               Price history\n";
    std::cout << "    RANGE_QUERY <sym> <type> <l> <r>     Range query on prices\n";
    std::cout << "    DEPENDENCY_PATH <sym1> <sym2>        Find dependency path\n";
    std::cout << "    OPTIMIZE_ORDER <side> <symbol> <qty> Optimize large order\n";
    std::cout << "    MARKET_SUMMARY                       Market overview\n";
    std::cout << "    STRESS_TEST <num_orders>             Run native speed test\n";
    std::cout << "\n";
    std::cout << "  General:\n";
    std::cout << "    HELP                                 Show this help\n";
    std::cout << "    EXIT                                 Quit\n";
    std::cout << "    DROP_EXIT                            Clear log files and quit\n";
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
//  Command Dispatcher
// ---------------------------------------------------------------------------

void dispatch(Exchange& exchange, Trie& trie, PriceTracker& priceTracker, MarketAnalytics& analytics, MarketGraph& graph,
              const std::vector<std::string>& tokens) {
    if (tokens.empty()) return;

    const std::string& cmd = tokens[0];

    // ---- BUY <symbol> <qty> <price> ----
    if (cmd == "BUY" && tokens.size() == 4) {
        int qty; double price;
        if (!Utils::toInt(tokens[2], qty) || qty <= 0) {
            std::cout << "Error: invalid quantity.\n"; return;
        }
        if (!Utils::toDouble(tokens[3], price) || price <= 0) {
            std::cout << "Error: invalid price.\n"; return;
        }
        std::string symbol = Utils::toUpper(tokens[1]);
        exchange.addOrder(symbol, "BUY", qty, price);
        trie.insert(symbol);
    }
    // ---- SELL <symbol> <qty> <price> ----
    else if (cmd == "SELL" && tokens.size() == 4) {
        int qty; double price;
        if (!Utils::toInt(tokens[2], qty) || qty <= 0) {
            std::cout << "Error: invalid quantity.\n"; return;
        }
        if (!Utils::toDouble(tokens[3], price) || price <= 0) {
            std::cout << "Error: invalid price.\n"; return;
        }
        std::string symbol = Utils::toUpper(tokens[1]);
        exchange.addOrder(symbol, "SELL", qty, price);
        trie.insert(symbol);
    }
    // ---- MARKET BUY|SELL <symbol> <qty> ----
    else if (cmd == "MARKET" && tokens.size() == 4) {
        std::string side = Utils::toUpper(tokens[1]);
        if (side != "BUY" && side != "SELL") {
            std::cout << "Error: MARKET requires BUY or SELL.\n"; return;
        }
        int qty;
        if (!Utils::toInt(tokens[3], qty) || qty <= 0) {
            std::cout << "Error: invalid quantity.\n"; return;
        }
        std::string symbol = Utils::toUpper(tokens[2]);
        exchange.addMarketOrder(symbol, side, qty);
        trie.insert(symbol);
    }
    // ---- CANCEL <orderID> ----
    else if (cmd == "CANCEL" && tokens.size() == 2) {
        int id;
        if (!Utils::toInt(tokens[1], id)) {
            std::cout << "Error: invalid order ID.\n"; return;
        }
        exchange.cancelOrder(id);
    }
    // ---- MODIFY <orderID> <side> <qty> <price> ----
    else if (cmd == "MODIFY" && tokens.size() == 5) {
        int id;
        if (!Utils::toInt(tokens[1], id)) {
            std::cout << "Error: invalid order ID.\n"; return;
        }
        std::string side = Utils::toUpper(tokens[2]);
        if (side != "BUY" && side != "SELL") {
            std::cout << "Error: side must be BUY or SELL.\n"; return;
        }
        int qty; double price;
        if (!Utils::toInt(tokens[3], qty) || qty <= 0) {
            std::cout << "Error: invalid quantity.\n"; return;
        }
        if (!Utils::toDouble(tokens[4], price) || price <= 0) {
            std::cout << "Error: invalid price.\n"; return;
        }
        exchange.modifyOrder(id, side, qty, price);
    }
    // ---- SHOW_ORDERBOOK [symbol] ----
    else if (cmd == "SHOW_ORDERBOOK") {
        if (tokens.size() >= 2)
            exchange.showOrderBook(Utils::toUpper(tokens[1]));
        else
            exchange.showOrderBook();
    }
    // ---- SHOW_TRADES ----
    else if (cmd == "SHOW_TRADES") {
        exchange.showTrades();
    }
    // ---- PERFORMANCE ----
    else if (cmd == "PERFORMANCE") {
        exchange.showPerformance();
    }
    // ---- SEARCH_SYMBOL <prefix> ----
    else if (cmd == "SEARCH_SYMBOL" && tokens.size() >= 2) {
        std::string prefix = Utils::toUpper(tokens[1]);
        auto results = trie.autocomplete(prefix);
        if (results.empty()) {
            std::cout << "  No matches for '" << prefix << "'\n";
        } else {
            std::cout << "\n  Matches for '" << prefix << "':\n";
            for (const auto& s : results) {
                std::cout << "    " << s << "\n";
            }
            std::cout << "  (" << results.size() << " result"
                      << (results.size() != 1 ? "s" : "") << ")\n\n";
        }
    }
    // ---- TOP_GAINERS ----
    else if (cmd == "TOP_GAINERS") {
        analytics.showTopGainers();
    }
    // ---- TOP_LOSERS ----
    else if (cmd == "TOP_LOSERS") {
        analytics.showTopLosers();
    }
    // ---- PRICE_HISTORY <symbol> ----
    else if (cmd == "PRICE_HISTORY" && tokens.size() >= 2) {
        priceTracker.showPriceHistory(Utils::toUpper(tokens[1]));
    }
    // ---- RANGE_QUERY <symbol> <type> <left> <right> ----
    else if (cmd == "RANGE_QUERY" && tokens.size() == 5) {
        int left, right;
        if (!Utils::toInt(tokens[3], left) || !Utils::toInt(tokens[4], right)) {
            std::cout << "Error: invalid range indices.\n"; return;
        }
        priceTracker.showRangeQuery(Utils::toUpper(tokens[1]), Utils::toUpper(tokens[2]), left, right);
    }
    // ---- DEPENDENCY_PATH <sym1> <sym2> ----
    else if (cmd == "DEPENDENCY_PATH" && tokens.size() == 3) {
        graph.findDependencyPath(Utils::toUpper(tokens[1]), Utils::toUpper(tokens[2]));
    }
    // ---- OPTIMIZE_ORDER <side> <symbol> <qty> ----
    else if (cmd == "OPTIMIZE_ORDER" && tokens.size() == 4) {
        int qty;
        if (!Utils::toInt(tokens[3], qty) || qty <= 0) {
            std::cout << "Error: invalid quantity.\n"; return;
        }
        optimizeOrder(Utils::toUpper(tokens[1]), Utils::toUpper(tokens[2]), qty);
    }
    // ---- MARKET_SUMMARY ----
    else if (cmd == "MARKET_SUMMARY") {
        analytics.showMarketSummary(exchange);
    }
    // ---- STRESS_TEST <qty> ----
    else if (cmd == "STRESS_TEST" && tokens.size() == 2) {
        int n;
        if (!Utils::toInt(tokens[1], n) || n <= 0) {
            std::cout << "Error: invalid number of orders.\n"; return;
        }

        std::vector<std::string> symbols = {"AAPL", "MSFT", "GOOG", "TSLA", "NVDA", "AMZN", "META", "NFLX", "AMD", "INTC"};
        std::ofstream outFile("data/stress_test_orders.txt");
        
        srand(static_cast<unsigned>(time(nullptr)));

        struct TestOrder { std::string side, sym; int qty; double price; };
        std::vector<TestOrder> testOrders;

        for (int i = 0; i < n; i++) {
            std::string side = (rand() % 2 == 0) ? "BUY" : "SELL";
            std::string sym = symbols[rand() % symbols.size()];
            int qty = (rand() % 50 + 1) * 10;
            double price = 150.0 + (((rand() % 1000) - 500) / 100.0);
            
            if (outFile.is_open()) {
                outFile << side << " " << sym << " " << qty << " " << std::fixed << std::setprecision(2) << price << "\n";
            }
            testOrders.push_back({side, sym, qty, price});
        }
        
        if (outFile.is_open()) outFile.close();
        
        std::cout << "\n  [STRESS TEST]\n";
        std::cout << "  Generated " << n << " orders and saved to data/stress_test_orders.txt\n";
        std::cout << "  Executing in memory...\n";

        exchange.muteOutput = true;
        
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto& o : testOrders) {
            int oldTradeCount = exchange.getTradeCount();
            
            exchange.addOrder(o.sym, o.side, o.qty, o.price);
            
            int newTradeCount = exchange.getTradeCount();
            if (newTradeCount > oldTradeCount) {
                const auto& trades = exchange.getTrades();
                for (int i = oldTradeCount; i < newTradeCount; i++) {
                    const auto& t = trades[i];
                    priceTracker.recordPrice(t.symbol, t.price, t.quantity);
                    analytics.recordTrade(t.symbol, t.price, t.quantity);
                }
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        double elapsedUs = std::chrono::duration<double, std::micro>(end - start).count();
        double elapsedMs = elapsedUs / 1000.0;
        
        exchange.muteOutput = false;

        std::cout << "  Done.\n";
        std::cout << "  Trades Executed: " << exchange.getTradeCount() << "\n";
        std::cout << "  Total Time: " << elapsedMs << " ms\n";
        std::cout << "  Avg Latency: " << std::fixed << std::setprecision(2) << elapsedUs / n << " us per order\n\n";
        
        std::ofstream histFile("data/stress_test_history.txt", std::ios::app);
        if (histFile.is_open()) {
            histFile << "Ran " << n << " orders. Avg latency: " << (elapsedUs / n) << " us. Total time: " << (elapsedMs) << " ms.\n";
        }
    }
    // ---- HELP ----
    else if (cmd == "HELP") {
        printHelp();
    }
    // ---- EXIT / DROP_EXIT ----
    else if (cmd == "EXIT" || cmd == "QUIT" || cmd == "DROP_EXIT") {
        // handled in main loop
    }
    // ---- Unknown ----
    else {
        std::cout << "Unknown command. Type HELP for available commands.\n";
    }
}

// ---------------------------------------------------------------------------
//  Main
// ---------------------------------------------------------------------------

int main() {
    printBanner();
    printHelp();

    Exchange exchange;

    // Initialize Trie with default symbols
    Trie trie;
    for (const auto& sym : DEFAULT_SYMBOLS) {
        trie.insert(sym);
    }
    // Also load from data file if it exists
    int loaded = trie.loadFromFile("data/stocks.txt");
    if (loaded > 0) {
        std::cout << "  Loaded " << loaded << " symbols from data/stocks.txt into Trie\n";
    }

    PriceTracker priceTracker;
    int pricesLoaded = priceTracker.loadFromFile("data/prices.txt");
    if (pricesLoaded > 0) {
        std::cout << "  Loaded historical prices for " << pricesLoaded << " symbols\n";
    }

    MarketAnalytics analytics;
    int analyticsLoaded = analytics.loadFromFile("data/stocks.txt");
    if (analyticsLoaded > 0) {
        std::cout << "  Loaded reference prices for " << analyticsLoaded << " symbols\n";
    }

    MarketGraph graph;
    int edgesLoaded = graph.loadFromFile("data/relationships.txt");
    if (edgesLoaded > 0) {
        std::cout << "  Loaded " << edgesLoaded << " market relationships\n";
    }
    std::cout << "\n";

    std::string line;

    while (true) {
        std::cout << "AlgoExchange> ";
        if (!std::getline(std::cin, line)) break;

        line = Utils::trim(line);
        if (line.empty()) continue;

        auto tokens = Utils::split(line);
        for (auto& t : tokens) t = Utils::toUpper(t);

        if (tokens[0] == "EXIT" || tokens[0] == "QUIT") {
            std::cout << "Shutting down. Goodbye!\n";
            break;
        }
        else if (tokens[0] == "DROP_EXIT") {
            std::ofstream ofs1("data/stress_test_orders.txt", std::ios::trunc);
            if (ofs1.is_open()) ofs1.close();
            std::ofstream ofs2("data/stress_test_history.txt", std::ios::trunc);
            if (ofs2.is_open()) ofs2.close();
            std::cout << "  Cleared generated history and order files.\n";
            std::cout << "  Shutting down. Goodbye!\n";
            break;
        }

        int oldTradeCount = exchange.getTradeCount();
        
        dispatch(exchange, trie, priceTracker, analytics, graph, tokens);
        
        int newTradeCount = exchange.getTradeCount();
        if (newTradeCount > oldTradeCount) {
            const auto& trades = exchange.getTrades();
            for (int i = oldTradeCount; i < newTradeCount; i++) {
                const auto& t = trades[i];
                priceTracker.recordPrice(t.symbol, t.price, t.quantity);
                analytics.recordTrade(t.symbol, t.price, t.quantity);
            }
        }
    }

    return 0;
}
