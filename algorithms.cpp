#pragma once

// ============================================================================
//  algorithms.cpp — Trie, Segment Tree, Graph, Dynamic Programming
// ============================================================================

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <cfloat>

// ============================================================================
//  Trie — Stock Symbol Autocomplete
// ============================================================================
//
//  Complexity:
//    Insert       O(L)          L = symbol length
//    Search       O(L)
//    StartsWith   O(L)
//    Autocomplete O(L + K)      K = number of matching results
//
// ============================================================================

struct TrieNode {
    std::unordered_map<char, TrieNode*> children;
    bool isEnd = false;

    ~TrieNode() {
        for (auto& [c, child] : children) {
            delete child;
        }
    }
};

class Trie {
    TrieNode* root;

    void collectWords(TrieNode* node, std::string& current,
                      std::vector<std::string>& results) const {
        if (node->isEnd) {
            results.push_back(current);
        }
        for (auto& [c, child] : node->children) {
            current.push_back(c);
            collectWords(child, current, results);
            current.pop_back();
        }
    }

public:
    Trie() : root(new TrieNode()) {}
    ~Trie() { delete root; }

    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    void insert(const std::string& word) {
        TrieNode* node = root;
        for (char c : word) {
            if (node->children.find(c) == node->children.end()) {
                node->children[c] = new TrieNode();
            }
            node = node->children[c];
        }
        node->isEnd = true;
    }

    bool search(const std::string& word) const {
        TrieNode* node = root;
        for (char c : word) {
            auto it = node->children.find(c);
            if (it == node->children.end()) return false;
            node = it->second;
        }
        return node->isEnd;
    }

    bool startsWith(const std::string& prefix) const {
        TrieNode* node = root;
        for (char c : prefix) {
            auto it = node->children.find(c);
            if (it == node->children.end()) return false;
            node = it->second;
        }
        return true;
    }

    std::vector<std::string> autocomplete(const std::string& prefix) const {
        std::vector<std::string> results;
        TrieNode* node = root;
        for (char c : prefix) {
            auto it = node->children.find(c);
            if (it == node->children.end()) return results;
            node = it->second;
        }
        std::string current = prefix;
        collectWords(node, current, results);
        std::sort(results.begin(), results.end());
        return results;
    }

    int loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return 0;
        int count = 0;
        std::string line;
        while (std::getline(file, line)) {
            auto tokens = Utils::split(line);
            if (!tokens.empty()) {
                insert(Utils::toUpper(tokens[0]));
                count++;
            }
        }
        return count;
    }
};

// ============================================================================
//  Segment Tree — Range Queries on Price History
// ============================================================================
//
//  Each node stores aggregated data for a range:
//    minPrice, maxPrice, priceSum, volumeSum, count
//
//  Complexity:
//    Build          O(N)
//    Point Update   O(log N)
//    Range Query    O(log N)
//
//  Supports: Range MAX, MIN, SUM, AVG, VOLUME, COUNT
//
// ============================================================================

struct SegNode {
    double minPrice  =  DBL_MAX;  // identity for min
    double maxPrice  = -DBL_MAX;  // identity for max
    double priceSum  = 0.0;
    int    volumeSum = 0;
    int    count     = 0;
};

class SegmentTree {
    std::vector<SegNode> tree;
    int capacity;

    SegNode merge(const SegNode& a, const SegNode& b) const {
        return {
            std::min(a.minPrice, b.minPrice),
            std::max(a.maxPrice, b.maxPrice),
            a.priceSum + b.priceSum,
            a.volumeSum + b.volumeSum,
            a.count + b.count
        };
    }

    void updateInternal(int idx, int start, int end, int pos,
                        double price, int volume) {
        if (start == end) {
            tree[idx] = {price, price, price, volume, 1};
            return;
        }
        int mid = (start + end) / 2;
        if (pos <= mid)
            updateInternal(2 * idx, start, mid, pos, price, volume);
        else
            updateInternal(2 * idx + 1, mid + 1, end, pos, price, volume);
        tree[idx] = merge(tree[2 * idx], tree[2 * idx + 1]);
    }

    SegNode queryInternal(int idx, int start, int end, int l, int r) const {
        if (r < start || end < l) return {};          // identity node
        if (l <= start && end <= r) return tree[idx];  // fully covered
        int mid = (start + end) / 2;
        return merge(queryInternal(2 * idx, start, mid, l, r),
                     queryInternal(2 * idx + 1, mid + 1, end, l, r));
    }

public:
    SegmentTree(int cap = 10000)
        : capacity(cap), tree(4 * cap) {}

    void update(int pos, double price, int volume) {
        if (pos < 0 || pos >= capacity) return;
        updateInternal(1, 0, capacity - 1, pos, price, volume);
    }

    SegNode query(int l, int r) const {
        if (l > r) return {};
        l = std::max(0, l);
        r = std::min(r, capacity - 1);
        return queryInternal(1, 0, capacity - 1, l, r);
    }
};

// ============================================================================
//  PriceTracker — Per-Symbol Price History with Segment Tree Queries
// ============================================================================

class PriceTracker {
    struct SymbolData {
        std::vector<double> prices;
        std::vector<int>    volumes;
        SegmentTree         segTree;
    };

    std::unordered_map<std::string, SymbolData> data;

public:
    // Record a new price + volume entry for a symbol
    void recordPrice(const std::string& symbol, double price, int volume) {
        auto& sd  = data[symbol];
        int   idx = static_cast<int>(sd.prices.size());
        sd.prices.push_back(price);
        sd.volumes.push_back(volume);
        sd.segTree.update(idx, price, volume);
    }

    // Load historical prices from file (SYMBOL p1 p2 p3 ...)
    int loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return 0;
        int count = 0;
        std::string line;
        while (std::getline(file, line)) {
            auto tokens = Utils::split(line);
            if (tokens.size() < 2) continue;
            std::string symbol = Utils::toUpper(tokens[0]);
            for (size_t i = 1; i < tokens.size(); i++) {
                double price;
                if (Utils::toDouble(tokens[i], price)) {
                    recordPrice(symbol, price, 0);
                    count++;
                }
            }
        }
        return count;
    }

    // Show full price history for a symbol
    void showPriceHistory(const std::string& symbol) const {
        auto it = data.find(symbol);
        if (it == data.end() || it->second.prices.empty()) {
            std::cout << "  No price data for " << symbol << "\n";
            return;
        }
        const auto& sd = it->second;
        Utils::printHeader("Price History: " + symbol);
        std::cout << "  " << sd.prices.size() << " entries\n\n";

        for (int i = 0; i < static_cast<int>(sd.prices.size()); i++) {
            std::cout << "  [" << std::setw(3) << i << "] "
                      << std::fixed << std::setprecision(2)
                      << std::setw(10) << sd.prices[i];
            if (sd.volumes[i] > 0)
                std::cout << "  vol=" << sd.volumes[i];
            if ((i + 1) % 4 == 0) std::cout << "\n";
        }
        if (sd.prices.size() % 4 != 0) std::cout << "\n";
    }

    // Run a range query and display result
    void showRangeQuery(const std::string& symbol, const std::string& type,
                        int l, int r) const {
        auto it = data.find(symbol);
        if (it == data.end() || it->second.prices.empty()) {
            std::cout << "  No price data for " << symbol << "\n";
            return;
        }
        const auto& sd = it->second;
        int maxIdx = static_cast<int>(sd.prices.size()) - 1;

        // Clamp range
        l = std::max(0, l);
        r = std::min(r, maxIdx);
        if (l > r) {
            std::cout << "  Invalid range [" << l << ".." << r << "]\n";
            return;
        }

        SegNode result = sd.segTree.query(l, r);

        std::cout << "\n  " << symbol << " Range [" << l << ".." << r << "]:\n";

        if (type == "MAX" || type == "ALL") {
            std::cout << "    MAX      : " << std::fixed << std::setprecision(2)
                      << result.maxPrice << "\n";
        }
        if (type == "MIN" || type == "ALL") {
            std::cout << "    MIN      : " << std::fixed << std::setprecision(2)
                      << result.minPrice << "\n";
        }
        if (type == "SUM" || type == "ALL") {
            std::cout << "    SUM      : " << std::fixed << std::setprecision(2)
                      << result.priceSum << "\n";
        }
        if (type == "AVG" || type == "ALL") {
            double avg = (result.count > 0) ? result.priceSum / result.count : 0.0;
            std::cout << "    AVG      : " << std::fixed << std::setprecision(2)
                      << avg << "\n";
        }
        if (type == "VOLUME" || type == "ALL") {
            std::cout << "    VOLUME   : " << result.volumeSum << "\n";
        }
        if (type == "COUNT" || type == "ALL") {
            std::cout << "    COUNT    : " << result.count << "\n";
        }

        if (type != "MAX" && type != "MIN" && type != "SUM" &&
            type != "AVG" && type != "VOLUME" && type != "COUNT" &&
            type != "ALL") {
            std::cout << "    Unknown query type '" << type << "'\n";
            std::cout << "    Valid: MAX, MIN, SUM, AVG, VOLUME, COUNT, ALL\n";
        }
        std::cout << "\n";
    }

    bool hasData(const std::string& symbol) const {
        return data.find(symbol) != data.end();
    }
};

// ============================================================================
//  Stubs — to be implemented in later phases
// ============================================================================

// ============================================================================
//  Graph — Market Relationships (Sector, Supplier, Correlation)
// ============================================================================
//
//  Complexity:
//    Build          O(V + E)
//    Dijkstra       O((V + E) log V)
//
// ============================================================================

#include <queue>

class MarketGraph {
    struct Edge {
        std::string target;
        std::string type;
        int         weight;
    };

    std::unordered_map<std::string, std::vector<Edge>> adjList;

public:
    // Load edges from relationships.txt (SYM1 SYM2 TYPE WEIGHT)
    int loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return 0;
        int count = 0;
        std::string line;
        while (std::getline(file, line)) {
            auto tokens = Utils::split(line);
            if (tokens.size() >= 4) {
                std::string u = Utils::toUpper(tokens[0]);
                std::string v = Utils::toUpper(tokens[1]);
                std::string type = Utils::toUpper(tokens[2]);
                int weight = 1;
                Utils::toInt(tokens[3], weight);

                adjList[u].push_back({v, type, weight});
                if (type == "CORRELATION") {
                    adjList[v].push_back({u, type, weight});
                }
                count++;
            }
        }
        return count;
    }

    // Dijkstra's algorithm for shortest dependency path
    void findDependencyPath(const std::string& start, const std::string& end) const {
        if (adjList.find(start) == adjList.end()) {
            std::cout << "  Start symbol '" << start << "' not in graph.\n\n";
            return;
        }
        if (adjList.find(end) == adjList.end()) {
            std::cout << "  Target symbol '" << end << "' not in graph.\n\n";
            return;
        }

        // min-heap: {distance, node}
        using PDI = std::pair<int, std::string>;
        std::priority_queue<PDI, std::vector<PDI>, std::greater<PDI>> pq;
        std::unordered_map<std::string, int> dist;
        std::unordered_map<std::string, std::string> parent;
        std::unordered_map<std::string, Edge> parentEdge;

        for (const auto& [node, edges] : adjList) {
            dist[node] = INT_MAX;
        }
        dist[start] = 0;
        pq.push({0, start});

        while (!pq.empty()) {
            auto [d, u] = pq.top();
            pq.pop();

            if (d > dist[u]) continue;
            if (u == end) break; // Found shortest path

            for (const auto& edge : adjList.at(u)) {
                const std::string& v = edge.target;
                int weight = edge.weight;

                if (dist.find(v) == dist.end()) dist[v] = INT_MAX;

                if (dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    parent[v] = u;
                    parentEdge[v] = edge;
                    pq.push({dist[v], v});
                }
            }
        }

        if (dist[end] == INT_MAX) {
            std::cout << "  No path found from " << start << " to " << end << ".\n\n";
            return;
        }

        // Reconstruct path
        std::vector<std::pair<std::string, Edge>> path;
        std::string curr = end;
        while (curr != start) {
            path.push_back({curr, parentEdge[curr]});
            curr = parent[curr];
        }
        std::reverse(path.begin(), path.end());

        Utils::printHeader("Dependency Path: " + start + " -> " + end);
        std::cout << "  Total Weight: " << dist[end] << "\n\n";
        
        std::cout << "  " << start << "\n";
        for (const auto& p : path) {
            std::cout << "    |--[" << p.second.type << " (w:" << p.second.weight << ")]--> " << p.first << "\n";
        }
        std::cout << "\n";
    }
};

// ============================================================================
//  Dynamic Programming — Execution Optimizer
// ============================================================================
//
//  Problem: Divide a large order (e.g. 10,000 shares) into standard lot
//  sizes to minimize total execution cost.
//
//  Cost model = Fixed Trade Fee + Market Impact
//  - Fixed fee encourages fewer, larger trades.
//  - Market impact (quadratic) encourages more, smaller trades.
//  DP naturally finds the optimal balance.
//
//  Complexity:
//    Time:  O(QTY * |Lots|)
//    Space: O(QTY)
//
// ============================================================================

void optimizeOrder(const std::string& side, const std::string& symbol, int qty) {
    if (qty <= 0 || qty > 100000) {
        std::cout << "  Error: Optimization only supported for 1 to 100,000 shares.\n\n";
        return;
    }

    // Standard lot sizes we are allowed to execute
    std::vector<int> lotSizes = {10, 50, 100, 200, 500, 1000, 5000};
    
    // Cost function for a single trade of 'size' shares
    auto getCost = [](int size) -> double {
        double fixedFee = 2.50; // $2.50 per trade
        double marketImpact = (size * size) * 0.00005; // Quadratic impact
        return fixedFee + marketImpact;
    };

    // dp[i] = min cost to execute exactly i shares
    std::vector<double> dp(qty + 1, DBL_MAX);
    // choice[i] = the lot size we chose to reach state i
    std::vector<int> choice(qty + 1, 0);

    dp[0] = 0.0;

    for (int i = 1; i <= qty; ++i) {
        for (int lot : lotSizes) {
            if (i >= lot && dp[i - lot] != DBL_MAX) {
                double cost = dp[i - lot] + getCost(lot);
                if (cost < dp[i]) {
                    dp[i] = cost;
                    choice[i] = lot;
                }
            }
        }
    }

    if (dp[qty] == DBL_MAX) {
        std::cout << "  Cannot execute " << qty << " shares using available lot sizes.\n\n";
        return;
    }

    // Reconstruct the optimal execution plan
    std::unordered_map<int, int> plan; // lotSize -> count
    int current = qty;
    while (current > 0) {
        int lot = choice[current];
        plan[lot]++;
        current -= lot;
    }

    Utils::printHeader("Execution Optimizer: " + side + " " + std::to_string(qty) + " " + symbol);
    std::cout << "  Model: Cost = $2.50 + (size^2 * 0.00005)\n\n";
    std::cout << "  Optimal Strategy:\n";
    
    std::vector<std::pair<int, int>> sortedPlan(plan.begin(), plan.end());
    std::sort(sortedPlan.rbegin(), sortedPlan.rend()); // Largest lots first

    for (const auto& [lot, count] : sortedPlan) {
        std::cout << "    " << std::setw(3) << count << " trades of " 
                  << std::setw(4) << lot << " shares  (Cost/trade: $"
                  << std::fixed << std::setprecision(2) << getCost(lot) << ")\n";
    }

    std::cout << "\n  Total Execution Cost: $" << std::fixed << std::setprecision(2) << dp[qty] << "\n\n";
}
