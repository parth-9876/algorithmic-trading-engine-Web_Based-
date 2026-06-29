#pragma once

// ============================================================================
//  analytics.cpp — Market Analytics, Heap Rankings, Summaries
// ============================================================================
//
//  StockStats   — per-symbol trade statistics
//  MarketAnalytics — heap-based rankings using std::priority_queue
//
//  Heap operations:
//    Top K Gainers     max-heap by % price change     O(N log N)
//    Top K Losers      min-heap by % price change     O(N log N)
//    Highest Volume    max-heap by total volume        O(N log N)
//    Most Active       max-heap by trade count         O(N log N)
//
// ============================================================================

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <functional>

// ---------------------------------------------------------------------------
//  MarketAnalytics
// ---------------------------------------------------------------------------

class MarketAnalytics {
    struct StockStats {
        std::string symbol;
        double referencePrice = 0.0;   // from stocks.txt or first trade
        double lastPrice      = 0.0;
        int    totalVolume    = 0;
        int    tradeCount     = 0;

        double pctChange() const {
            if (referencePrice <= 0.0) return 0.0;
            return (lastPrice - referencePrice) / referencePrice * 100.0;
        }
    };

    std::unordered_map<std::string, StockStats> stats;

public:
    // Set reference price from stocks.txt (before any trades)
    void setReferencePrice(const std::string& symbol, double price) {
        auto& s = stats[symbol];
        s.symbol         = symbol;
        s.referencePrice = price;
        s.lastPrice      = price;
    }

    // Record a live trade
    void recordTrade(const std::string& symbol, double price, int qty) {
        auto& s = stats[symbol];
        if (s.symbol.empty()) s.symbol = symbol;
        if (s.referencePrice <= 0.0) s.referencePrice = price;
        s.lastPrice    = price;
        s.totalVolume += qty;
        s.tradeCount++;
    }

    // Load reference prices from stocks.txt (SYMBOL COMPANY SECTOR PRICE)
    int loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return 0;
        int count = 0;
        std::string line;
        while (std::getline(file, line)) {
            auto tokens = Utils::split(line);
            if (tokens.size() >= 4) {
                double price;
                if (Utils::toDouble(tokens[3], price)) {
                    setReferencePrice(Utils::toUpper(tokens[0]), price);
                    count++;
                }
            }
        }
        return count;
    }

    // -------------------------------------------------------------------
    //  Top K Gainers — max-heap by % change
    // -------------------------------------------------------------------
    void showTopGainers(int k = 10) const {
        // Build a max-heap by pctChange
        auto cmp = [](const StockStats& a, const StockStats& b) {
            return a.pctChange() < b.pctChange();  // max-heap
        };
        std::priority_queue<StockStats, std::vector<StockStats>,
                            decltype(cmp)> heap(cmp);

        for (const auto& [sym, s] : stats) {
            if (s.tradeCount > 0 && s.pctChange() > 0.0)
                heap.push(s);
        }

        Utils::printHeader("Top Gainers");
        if (heap.empty()) {
            std::cout << "  (no gainers yet)\n\n";
            return;
        }
        std::cout << "  " << std::left << std::setw(6) << "#"
                  << std::setw(8) << "Symbol"
                  << std::setw(12) << "Change"
                  << std::setw(12) << "Last"
                  << std::setw(12) << "Ref" << "\n";
        Utils::printSeparator(50);

        int shown = 0;
        while (!heap.empty() && shown < k) {
            auto s = heap.top(); heap.pop();
            std::cout << "  " << std::left << std::setw(6) << (shown + 1)
                      << std::setw(8)  << s.symbol
                      << "+" << std::fixed << std::setprecision(2)
                      << std::setw(10) << s.pctChange() << "%"
                      << std::setw(12) << s.lastPrice
                      << std::setw(12) << s.referencePrice << "\n";
            shown++;
        }
        std::cout << "\n";
    }

    // -------------------------------------------------------------------
    //  Top K Losers — max-heap by negative % change
    // -------------------------------------------------------------------
    void showTopLosers(int k = 10) const {
        auto cmp = [](const StockStats& a, const StockStats& b) {
            return a.pctChange() > b.pctChange();  // min-heap (biggest losers first)
        };
        std::priority_queue<StockStats, std::vector<StockStats>,
                            decltype(cmp)> heap(cmp);

        for (const auto& [sym, s] : stats) {
            if (s.tradeCount > 0 && s.pctChange() < 0.0)
                heap.push(s);
        }

        Utils::printHeader("Top Losers");
        if (heap.empty()) {
            std::cout << "  (no losers yet)\n\n";
            return;
        }
        std::cout << "  " << std::left << std::setw(6) << "#"
                  << std::setw(8) << "Symbol"
                  << std::setw(12) << "Change"
                  << std::setw(12) << "Last"
                  << std::setw(12) << "Ref" << "\n";
        Utils::printSeparator(50);

        int shown = 0;
        while (!heap.empty() && shown < k) {
            auto s = heap.top(); heap.pop();
            std::cout << "  " << std::left << std::setw(6) << (shown + 1)
                      << std::setw(8)  << s.symbol
                      << std::fixed << std::setprecision(2)
                      << std::setw(11) << s.pctChange() << "%"
                      << std::setw(12) << s.lastPrice
                      << std::setw(12) << s.referencePrice << "\n";
            shown++;
        }
        std::cout << "\n";
    }

    // -------------------------------------------------------------------
    //  Market Summary — overview with top entries from each category
    // -------------------------------------------------------------------
    void showMarketSummary(const Exchange& exchange) const {
        Utils::printHeader("Market Summary");

        // Overall stats
        int tradedSymbols = 0;
        int totalVolume   = 0;
        int totalTrades   = 0;
        for (const auto& [sym, s] : stats) {
            if (s.tradeCount > 0) {
                tradedSymbols++;
                totalVolume += s.totalVolume;
                totalTrades += s.tradeCount;
            }
        }

        std::cout << "  Symbols traded     : " << tradedSymbols << "\n";
        std::cout << "  Total orders       : " << exchange.getOrderCount() << "\n";
        std::cout << "  Total trades       : " << exchange.getTradeCount() << "\n";
        std::cout << "  Total volume       : " << totalVolume << " shares\n";
        std::cout << "\n";

        // Collect traded stocks into a vector for sorting
        std::vector<StockStats> traded;
        for (const auto& [sym, s] : stats) {
            if (s.tradeCount > 0) traded.push_back(s);
        }

        if (traded.empty()) {
            std::cout << "  No trades to analyze yet.\n\n";
            return;
        }

        // Top Gainer
        auto bestGainer = std::max_element(traded.begin(), traded.end(),
            [](const StockStats& a, const StockStats& b) {
                return a.pctChange() < b.pctChange();
            });
        std::cout << "  Best Gainer        : " << bestGainer->symbol
                  << "  " << std::showpos << std::fixed << std::setprecision(2)
                  << bestGainer->pctChange() << "%" << std::noshowpos << "\n";

        // Top Loser
        auto worstLoser = std::min_element(traded.begin(), traded.end(),
            [](const StockStats& a, const StockStats& b) {
                return a.pctChange() < b.pctChange();
            });
        std::cout << "  Worst Loser        : " << worstLoser->symbol
                  << "  " << std::showpos << std::fixed << std::setprecision(2)
                  << worstLoser->pctChange() << "%" << std::noshowpos << "\n";

        // Highest Volume (max-heap)
        auto highVol = std::max_element(traded.begin(), traded.end(),
            [](const StockStats& a, const StockStats& b) {
                return a.totalVolume < b.totalVolume;
            });
        std::cout << "  Highest Volume     : " << highVol->symbol
                  << "  " << highVol->totalVolume << " shares\n";

        // Most Active (max-heap by trade count)
        auto mostActive = std::max_element(traded.begin(), traded.end(),
            [](const StockStats& a, const StockStats& b) {
                return a.tradeCount < b.tradeCount;
            });
        std::cout << "  Most Active        : " << mostActive->symbol
                  << "  " << mostActive->tradeCount << " trades\n";

        std::cout << "\n";
    }
};
