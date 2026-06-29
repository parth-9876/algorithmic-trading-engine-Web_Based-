#pragma once

// ============================================================================
//  utils.cpp — Parsing, Formatting, Performance Measurement
// ============================================================================

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cctype>

namespace Utils {

// ---------------------------------------------------------------------------
//  String Utilities
// ---------------------------------------------------------------------------

// Remove leading/trailing whitespace
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Convert string to uppercase
std::string toUpper(const std::string& s) {
    std::string result = s;
    for (auto& c : result) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return result;
}

// Split string by whitespace into tokens
std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Safe string-to-int conversion, returns false on failure
bool toInt(const std::string& s, int& out) {
    try {
        size_t pos = 0;
        out = std::stoi(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

// Safe string-to-double conversion, returns false on failure
bool toDouble(const std::string& s, double& out) {
    try {
        size_t pos = 0;
        out = std::stod(s, &pos);
        return pos == s.size();
        } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
//  Performance Timer
// ---------------------------------------------------------------------------

class Timer {
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    TimePoint           startTime;
    std::vector<double> latencies;   // microseconds per operation

public:
    void start() {
        startTime = Clock::now();
    }

    double stop() {
        auto end = Clock::now();
        double us = std::chrono::duration<double, std::micro>(end - startTime).count();
        latencies.push_back(us);
        return us;
    }

    int getCount() const {
        return static_cast<int>(latencies.size());
    }

    double getAverage() const {
        if (latencies.empty()) return 0.0;
        double sum = 0.0;
        for (double l : latencies) sum += l;
        return sum / static_cast<double>(latencies.size());
    }

    double getWorst() const {
        if (latencies.empty()) return 0.0;
        return *std::max_element(latencies.begin(), latencies.end());
    }
};

// ---------------------------------------------------------------------------
//  Display Helpers
// ---------------------------------------------------------------------------

void printSeparator(int width = 60) {
    std::cout << std::string(width, '-') << "\n";
}

void printHeader(const std::string& title, int width = 60) {
    std::cout << "\n";
    std::cout << std::string(width, '=') << "\n";
    int pad = (width - static_cast<int>(title.size())) / 2;
    if (pad > 0) std::cout << std::string(pad, ' ');
    std::cout << title << "\n";
    std::cout << std::string(width, '=') << "\n";
}

} // namespace Utils
