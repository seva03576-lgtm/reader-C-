#pragma once

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

class Timer {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Ms        = std::chrono::milliseconds;
    using Us        = std::chrono::microseconds;

    Timer() : start_(Clock::now()), stopped_(false) {}

    void reset() {
        start_   = Clock::now();
        stopped_ = false;
    }

    void stop() {
        end_     = Clock::now();
        stopped_ = true;
    }

    [[nodiscard]] double elapsed_ms() const {
        auto endpoint = stopped_ ? end_ : Clock::now();
        return static_cast<double>(
            std::chrono::duration_cast<Us>(endpoint - start_).count()
        ) / 1000.0;
    }

    [[nodiscard]] double elapsed_sec() const {
        return elapsed_ms() / 1000.0;
    }

    [[nodiscard]] std::string elapsed_str() const {
        double ms = elapsed_ms();
        std::ostringstream oss;
        if (ms < 1000.0) {
            oss << std::fixed << std::setprecision(2) << ms << " ms";
        } else {
            oss << std::fixed << std::setprecision(3) << (ms / 1000.0) << " s";
        }
        return oss.str();
    }

private:
    TimePoint start_;
    TimePoint end_;
    bool      stopped_;
};