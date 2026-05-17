#pragma once

#include "thread_pool.hpp"
#include "progress_bar.hpp"
#include "timer.hpp"

#include <filesystem>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <fstream>

namespace fs = std::filesystem;

struct ParseConfig {
    fs::path    input_path;
    fs::path    output_path;
    std::string keyword;
    std::size_t thread_count;
    std::size_t chunk_size;
};

struct ParseResult {
    std::uint64_t total_lines;
    std::uint64_t matched_lines;
    std::uint64_t total_bytes;
    double        elapsed_ms;
    double        speed_mb_sec;
};

struct Chunk {
    std::uint64_t offset;
    std::uint64_t size;
    std::size_t   index;
};

class LogParser {
public:
    explicit LogParser(ParseConfig config);

    ParseResult run();

private:
    std::vector<Chunk> split_file() const;

    void process_chunk(
        const Chunk&      chunk,
        ProgressBar&      bar,
        const Timer&      timer
    );

    ParseConfig   config_;
    std::mutex    output_mutex_;
    std::ofstream output_file_;

    std::atomic<std::uint64_t> total_lines_   {0};
    std::atomic<std::uint64_t> matched_lines_ {0};
};