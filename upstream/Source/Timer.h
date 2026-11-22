#pragma once

#include <chrono>

class Timer {
public:
    Timer() = default;
    bool boop();
    void reset();
private:
    bool started = false;
    std::chrono::time_point<std::chrono::steady_clock> lastBoop;
};
