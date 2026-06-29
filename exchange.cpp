#pragma once

// ============================================================================
//  exchange.cpp — Order Book, Matching Engine, Trade History
// ============================================================================
//
//  Data Structures:
//    - std::set<Order, BuyComparator>   per symbol  (buy side)
//    - std::set<Order, SellComparator>  per symbol  (sell side)
//    - std::unordered_map<int, BuyRef>  for O(1) buy order lookup
//    - std::unordered_map<int, SellRef> for O(1) sell order lookup
//
//  Complexity:
//    Insert     O(log N)
//    Cancel     O(log N)   — O(1) map lookup + O(log N) set erase
//    Modify     O(log N)   — erase + re-insert
//    Best Bid   O(1)       — set::begin()
//    Best Ask   O(1)       — set::begin()
//    Match      O(log N)   per fill
//
//  Why std::set over std::priority_queue?
//    - set supports O(log N) erase-by-iterator (needed for cancel/modify)
//    - set supports iteration (needed for order book display)
//    - priority_queue only exposes top and has no erase
//
// ============================================================================

#include <set>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <cfloat>

// ---------------------------------------------------------------------------
//  Order
// ---------------------------------------------------------------------------

struct Order {
    int         id;
    std::string symbol;
    std::string side;       // "BUY" or "SELL"
    int         quantity;
    double      price;
    long long   timestamp;
};

// ---------------------------------------------------------------------------
//  Trade
// ---------------------------------------------------------------------------

struct Trade {
    int         tradeId;
    std::string symbol;
    int         buyOrderId;
    int         sellOrderId;
    int         quantity;
    double      price;
    long long   timestamp;
};

// ---------------------------------------------------------------------------
//  Comparators  (strict weak ordering for std::set)
//
//  BuyComparator  — highest price first, then earliest timestamp (FIFO)
//  SellComparator — lowest  price first, then earliest timestamp (FIFO)
//  Order ID as final tiebreaker guarantees uniqueness in the set.
// ---------------------------------------------------------------------------

struct BuyComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price > b.price) return true;
        if (a.price < b.price) return false;
        if (a.timestamp < b.timestamp) return true;
        if (a.timestamp > b.timestamp) return false;
        return a.id < b.id;
    }
};

struct SellComparator {
    bool operator()(const Order& a, const Order& b) const {
        if (a.price < b.price) return true;
        if (a.price > b.price) return false;
        if (a.timestamp < b.timestamp) return true;
        if (a.timestamp > b.timestamp) return false;
        return a.id < b.id;
    }
};

using BuyBook  = std::set<Order, BuyComparator>;
using SellBook = std::set<Order, SellComparator>;

// ---------------------------------------------------------------------------
//  Exchange
// ---------------------------------------------------------------------------

class Exchange {
    // Per-symbol order books
    std::unordered_map<std::string, BuyBook>  buyBooks;
    std::unordered_map<std::string, SellBook> sellBooks;

    // Fast order lookup — maps OrderID to {symbol, iterator}
    struct BuyRef {
        std::string       symbol;
        BuyBook::iterator it;
    };
    struct SellRef {
        std::string        symbol;
        SellBook::iterator it;
    };

    std::unordered_map<int, BuyRef>  buyIndex;
    std::unordered_map<int, SellRef> sellIndex;

    // Trade log
    std::vector<Trade> trades;

    // Counters
    int       nextOrderId    = 1;
    int       nextTradeId    = 1;
    long long timestampCounter = 0;

    // Performance tracking
    Utils::Timer timer;

public:
    bool muteOutput = false;

    // ----- internal helpers ------------------------------------------------

    void recordTrade(const std::string& symbol, int buyId, int sellId,
                     int qty, double price) {
        trades.push_back({nextTradeId++, symbol, buyId, sellId, qty, price,
                          timestampCounter});
        if (!muteOutput) {
            std::cout << "  >> Trade #" << (nextTradeId - 1) << ": " << symbol
                      << "  Qty=" << qty
                      << "  Price=" << std::fixed << std::setprecision(2) << price
                      << "  (Buy #" << buyId << " x Sell #" << sellId << ")\n";
        }
    }

    // Match crossing orders for a given symbol.
    // Called after every insert or modify.
    void matchOrders(const std::string& symbol) {
        auto& buys  = buyBooks[symbol];
        auto& sells = sellBooks[symbol];

        while (!buys.empty() && !sells.empty()) {
            auto bestBuy  = buys.begin();
            auto bestSell = sells.begin();

            // No match if best buy price < best sell price
            if (bestBuy->price < bestSell->price) break;

            int matchQty = std::min(bestBuy->quantity, bestSell->quantity);

            // Trade price = resting (maker) order's price.
            // The order with the earlier timestamp was already in the book.
            double tradePrice = (bestBuy->timestamp < bestSell->timestamp) ? bestBuy->price : bestSell->price;
            
            // Handle MARKET order prices (DBL_MAX for buy, 0.0 for sell)
            if (tradePrice >= DBL_MAX / 2.0 || tradePrice <= 0.001) {
                tradePrice = (bestBuy->timestamp < bestSell->timestamp) ? bestSell->price : bestBuy->price;
                if (tradePrice >= DBL_MAX / 2.0 || tradePrice <= 0.001) {
                    tradePrice = 0.0; // Both were market orders
                }
            }

            recordTrade(symbol, bestBuy->id, bestSell->id, matchQty, tradePrice);

            int buyRemaining  = bestBuy->quantity  - matchQty;
            int sellRemaining = bestSell->quantity - matchQty;
            int buyId         = bestBuy->id;
            int sellId        = bestSell->id;
            Order buyOrder    = *bestBuy;
            Order sellOrder   = *bestSell;

            // Remove matched orders from book + index
            buys.erase(bestBuy);
            buyIndex.erase(buyId);

            sells.erase(bestSell);
            sellIndex.erase(sellId);

            // Re-insert partially filled orders
            if (buyRemaining > 0) {
                buyOrder.quantity = buyRemaining;
                auto [it, ok] = buys.insert(buyOrder);
                buyIndex[buyId] = {symbol, it};
            }
            if (sellRemaining > 0) {
                sellOrder.quantity = sellRemaining;
                auto [it, ok] = sells.insert(sellOrder);
                sellIndex[sellId] = {symbol, it};
            }
        }
    }

public:
    // -------------------------------------------------------------------
    //  Add a limit order.  Returns assigned order ID.
    // -------------------------------------------------------------------
    int addOrder(const std::string& symbol, const std::string& side,
                 int quantity, double price) {
        timer.start();

        int       orderId = nextOrderId++;
        long long ts      = timestampCounter++;
        Order     order{orderId, symbol, side, quantity, price, ts};

        if (side == "BUY") {
            auto [it, ok] = buyBooks[symbol].insert(order);
            buyIndex[orderId] = {symbol, it};
        } else {
            auto [it, ok] = sellBooks[symbol].insert(order);
            sellIndex[orderId] = {symbol, it};
        }

        matchOrders(symbol);

        double elapsed = timer.stop();
        if (!muteOutput) {
            std::cout << "[Order #" << orderId << "] " << side << " " << quantity
                      << " " << symbol << " @ ";
            if (price >= DBL_MAX / 2.0)
                std::cout << "MARKET";
            else if (price <= 0.001)
                std::cout << "MARKET";
            else
                std::cout << std::fixed << std::setprecision(2) << price;
            std::cout << "  (" << std::fixed << std::setprecision(1) << elapsed << " us)\n";
        }
        return orderId;
    }

    // -------------------------------------------------------------------
    //  Add a market order (no price limit — fills at best available).
    // -------------------------------------------------------------------
    int addMarketOrder(const std::string& symbol, const std::string& side,
                       int quantity) {
        double aggressivePrice = (side == "BUY") ? DBL_MAX : 0.0;
        return addOrder(symbol, side, quantity, aggressivePrice);
    }

    // -------------------------------------------------------------------
    //  Cancel an order by ID.
    // -------------------------------------------------------------------
    bool cancelOrder(int orderId) {
        timer.start();

        auto buyIt = buyIndex.find(orderId);
        if (buyIt != buyIndex.end()) {
            buyBooks[buyIt->second.symbol].erase(buyIt->second.it);
            buyIndex.erase(buyIt);
            double elapsed = timer.stop();
            std::cout << "[Cancel #" << orderId << "] Success  ("
                      << std::fixed << std::setprecision(1) << elapsed << " us)\n";
            return true;
        }

        auto sellIt = sellIndex.find(orderId);
        if (sellIt != sellIndex.end()) {
            sellBooks[sellIt->second.symbol].erase(sellIt->second.it);
            sellIndex.erase(sellIt);
            double elapsed = timer.stop();
            std::cout << "[Cancel #" << orderId << "] Success  ("
                      << std::fixed << std::setprecision(1) << elapsed << " us)\n";
            return true;
        }

        timer.stop();
        std::cout << "[Cancel #" << orderId << "] Error: order not found\n";
        return false;
    }

    // -------------------------------------------------------------------
    //  Modify an existing order (cancel + re-insert with new timestamp).
    // -------------------------------------------------------------------
    bool modifyOrder(int orderId, const std::string& newSide,
                     int newQty, double newPrice) {
        timer.start();

        std::string symbol;

        // Find and remove the old order
        auto buyIt = buyIndex.find(orderId);
        if (buyIt != buyIndex.end()) {
            symbol = buyIt->second.symbol;
            buyBooks[symbol].erase(buyIt->second.it);
            buyIndex.erase(buyIt);
        } else {
            auto sellIt = sellIndex.find(orderId);
            if (sellIt != sellIndex.end()) {
                symbol = sellIt->second.symbol;
                sellBooks[symbol].erase(sellIt->second.it);
                sellIndex.erase(sellIt);
            } else {
                timer.stop();
                std::cout << "[Modify #" << orderId << "] Error: order not found\n";
                return false;
            }
        }

        // Re-insert with same ID, new params, new timestamp (loses time priority)
        long long ts = timestampCounter++;
        Order newOrder{orderId, symbol, newSide, newQty, newPrice, ts};

        if (newSide == "BUY") {
            auto [it, ok] = buyBooks[symbol].insert(newOrder);
            buyIndex[orderId] = {symbol, it};
        } else {
            auto [it, ok] = sellBooks[symbol].insert(newOrder);
            sellIndex[orderId] = {symbol, it};
        }

        matchOrders(symbol);

        double elapsed = timer.stop();
        std::cout << "[Modify #" << orderId << "] " << newSide << " " << newQty
                  << " " << symbol << " @ "
                  << std::fixed << std::setprecision(2) << newPrice
                  << "  (" << std::setprecision(1) << elapsed << " us)\n";
        return true;
    }

    // -------------------------------------------------------------------
    //  Display order book (all symbols or a specific one).
    // -------------------------------------------------------------------
    void showOrderBook(const std::string& filterSymbol = "") const {
        std::set<std::string> symbols;
        if (filterSymbol.empty()) {
            for (const auto& [sym, _] : buyBooks)  symbols.insert(sym);
            for (const auto& [sym, _] : sellBooks) symbols.insert(sym);
        } else {
            symbols.insert(filterSymbol);
        }

        if (symbols.empty()) {
            std::cout << "Order book is empty.\n";
            return;
        }

        for (const auto& sym : symbols) {
            Utils::printHeader(sym);

            // Buy side (bids)
            std::cout << "  BUY (Bids)\n";
            auto buyIt = buyBooks.find(sym);
            if (buyIt != buyBooks.end() && !buyIt->second.empty()) {
                std::cout << "  " << std::left
                          << std::setw(8)  << "ID"
                          << std::setw(14) << "Price"
                          << std::setw(10) << "Qty" << "\n";
                Utils::printSeparator(34);
                for (const auto& o : buyIt->second) {
                std::cout << "  " << std::left
                          << std::setw(8)  << o.id;
                if (o.price >= DBL_MAX / 2.0) {
                    std::cout << std::setw(14) << "MARKET";
                } else {
                    std::cout << std::setw(14) << std::fixed << std::setprecision(2) << o.price;
                }
                std::cout << std::setw(10) << o.quantity << "\n";
            }
            } else {
                std::cout << "  (empty)\n";
            }

            std::cout << "\n";

            // Sell side (asks)
            std::cout << "  SELL (Asks)\n";
            auto sellIt = sellBooks.find(sym);
            if (sellIt != sellBooks.end() && !sellIt->second.empty()) {
                std::cout << "  " << std::left
                          << std::setw(8)  << "ID"
                          << std::setw(14) << "Price"
                          << std::setw(10) << "Qty" << "\n";
                Utils::printSeparator(34);
                for (const auto& o : sellIt->second) {
                    std::cout << "  " << std::left
                              << std::setw(8)  << o.id;
                    if (o.price <= 0.001) {
                        std::cout << std::setw(14) << "MARKET";
                    } else {
                        std::cout << std::setw(14) << std::fixed << std::setprecision(2) << o.price;
                    }
                    std::cout << std::setw(10) << o.quantity << "\n";
                }
            } else {
                std::cout << "  (empty)\n";
            }
            std::cout << "\n";
        }
    }

    // -------------------------------------------------------------------
    //  Display trade history.
    // -------------------------------------------------------------------
    void showTrades() const {
        if (trades.empty()) {
            std::cout << "No trades executed yet.\n";
            return;
        }

        Utils::printHeader("Trade History");
        std::cout << std::left
                  << std::setw(8)  << "ID"
                  << std::setw(8)  << "Symbol"
                  << std::setw(10) << "BuyID"
                  << std::setw(10) << "SellID"
                  << std::setw(10) << "Qty"
                  << std::setw(12) << "Price" << "\n";
        Utils::printSeparator(58);

        for (const auto& t : trades) {
            std::cout << std::left
                      << std::setw(8)  << t.tradeId
                      << std::setw(8)  << t.symbol
                      << std::setw(10) << t.buyOrderId
                      << std::setw(10) << t.sellOrderId
                      << std::setw(10) << t.quantity
                      << std::setw(12) << std::fixed << std::setprecision(2) << t.price
                      << "\n";
        }
    }

    // -------------------------------------------------------------------
    //  Display performance stats.
    // -------------------------------------------------------------------
    void showPerformance() const {
        Utils::printHeader("Performance");
        std::cout << "  Operations processed : " << timer.getCount() << "\n"
                  << "  Trades executed      : " << (nextTradeId - 1) << "\n"
                  << "  Avg latency          : " << std::fixed << std::setprecision(2)
                  << timer.getAverage() << " us\n"
                  << "  Worst latency        : " << timer.getWorst() << " us\n";
    }

    // -------------------------------------------------------------------
    //  Accessors (used by analytics module in later phases)
    // -------------------------------------------------------------------
    const std::vector<Trade>& getTrades() const { return trades; }
    int getOrderCount() const { return nextOrderId - 1; }
    int getTradeCount() const { return nextTradeId - 1; }

    const std::unordered_map<std::string, BuyBook>& getBuyBooks()   const { return buyBooks; }
    const std::unordered_map<std::string, SellBook>& getSellBooks() const { return sellBooks; }
};
