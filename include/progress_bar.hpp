#pragma once

#include <atomic>
#include <string>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <mutex>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/ioctl.h>
    #include <unistd.h>
#endif

class ProgressBar {
public:
    explicit ProgressBar(std::uint64_t total_bytes)
        : total_(total_bytes)
        , processed_(0)
        , matches_(0)
        , finished_(false)
    {}

    void update(std::uint64_t bytes_processed, std::uint64_t new_matches) {
        processed_.fetch_add(bytes_processed, std::memory_order_relaxed);
        matches_.fetch_add(new_matches,       std::memory_order_relaxed);
    }

    void finish() {
        finished_.store(true, std::memory_order_relaxed);
    }

    void render(double elapsed_sec) const {
        std::uint64_t proc  = processed_.load(std::memory_order_relaxed);
        std::uint64_t match = matches_.load(std::memory_order_relaxed);

        double fraction = (total_ > 0)
            ? static_cast<double>(proc) / static_cast<double>(total_)
            : 1.0;
        if (fraction > 1.0) fraction = 1.0;

        int bar_width = terminal_width() - 45;
        if (bar_width < 10) bar_width = 10;

        int filled = static_cast<int>(fraction * bar_width);

        double speed_mb = (elapsed_sec > 0.0)
            ? (static_cast<double>(proc) / (1024.0 * 1024.0)) / elapsed_sec
            : 0.0;

        std::ostringstream oss;
        oss << "\r\033[36m[\033[0m";

        for (int i = 0; i < bar_width; ++i) {
            if (i < filled) {
                oss << "\033[32m█\033[0m";
            } else if (i == filled) {
                oss << "\033[33m▓\033[0m";
            } else {
                oss << "\033[90m░\033[0m";
            }
        }

        oss << "\033[36m]\033[0m"
            << " \033[97m" << std::setw(3) << static_cast<int>(fraction * 100.0) << "%\033[0m"
            << " | \033[93m" << std::fixed << std::setprecision(2) << speed_mb << " MB/s\033[0m"
            << " | \033[91mMatches: " << match << "\033[0m";

        std::cout << oss.str() << std::flush;
    }

private:
    static int terminal_width() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            return csbi.srWindow.Right - csbi.srWindow.Left + 1;
        }
        return 80;
#else
        struct winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
            return ws.ws_col;
        }
        return 80;
#endif
    }

    std::uint64_t            total_;
    std::atomic<std::uint64_t> processed_;
    std::atomic<std::uint64_t> matches_;
    std::atomic<bool>          finished_;
};