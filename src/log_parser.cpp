#include "log_parser.hpp"

#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <string_view>
#include <vector>
#include <future>
#include <thread>
#include <iostream>
#include <cstring>

static constexpr std::size_t DEFAULT_CHUNK_SIZE = 32 * 1024 * 1024;

LogParser::LogParser(ParseConfig config) : config_(std::move(config)) {
    if (!fs::exists(config_.input_path)) {
        throw std::runtime_error(
            "Input file not found: " + config_.input_path.string()
        );
    }

    if (config_.chunk_size == 0) {
        config_.chunk_size = DEFAULT_CHUNK_SIZE;
    }

    if (config_.thread_count == 0) {
        config_.thread_count = std::thread::hardware_concurrency();
        if (config_.thread_count == 0) config_.thread_count = 4;
    }

    output_file_.open(config_.output_path, std::ios::out | std::ios::trunc);
    if (!output_file_.is_open()) {
        throw std::runtime_error(
            "Cannot open output file: " + config_.output_path.string()
        );
    }
}

std::vector<Chunk> LogParser::split_file() const {
    std::vector<Chunk> chunks;
    std::uint64_t file_size = fs::file_size(config_.input_path);
    if (file_size == 0) return chunks;

    std::uint64_t offset = 0;
    std::size_t   index  = 0;

    std::ifstream probe(config_.input_path, std::ios::binary);
    if (!probe.is_open()) {
        throw std::runtime_error("Cannot open input file for splitting");
    }

    while (offset < file_size) {
        std::uint64_t end = std::min(
            offset + static_cast<std::uint64_t>(config_.chunk_size),
            file_size
        );

        if (end < file_size) {
            probe.seekg(static_cast<std::streamoff>(end));
            char c = 0;
            while (probe.get(c) && c != '\n') {
                ++end;
            }
            if (c == '\n') ++end;
        }

        chunks.push_back({offset, end - offset, index++});
        offset = end;
    }

    return chunks;
}

void LogParser::process_chunk(
    const Chunk&  chunk,
    ProgressBar&  bar,
    const Timer&  timer
) {
    std::ifstream file(config_.input_path, std::ios::binary);
    if (!file.is_open()) return;

    file.seekg(static_cast<std::streamoff>(chunk.offset));

    std::vector<char> buffer(chunk.size);
    file.read(buffer.data(), static_cast<std::streamsize>(chunk.size));
    std::uint64_t bytes_read = static_cast<std::uint64_t>(file.gcount());

    std::string_view data(buffer.data(), bytes_read);
    std::string_view keyword(config_.keyword);

    std::vector<std::string> local_matches;
    local_matches.reserve(128);

    std::uint64_t local_lines   = 0;
    std::uint64_t local_matches_count = 0;

    std::size_t pos   = 0;
    std::size_t start = 0;

    while (pos <= data.size()) {
        if (pos == data.size() || data[pos] == '\n') {
            if (pos > start) {
                std::string_view line = data.substr(start, pos - start);
                ++local_lines;

                if (line.find(keyword) != std::string_view::npos) {
                    ++local_matches_count;
                    local_matches.emplace_back(line);
                    local_matches.back() += '\n';
                }
            }
            start = pos + 1;
        }
        ++pos;
    }

    total_lines_.fetch_add(local_lines,         std::memory_order_relaxed);
    matched_lines_.fetch_add(local_matches_count, std::memory_order_relaxed);

    if (!local_matches.empty()) {
        std::unique_lock<std::mutex> lock(output_mutex_);
        for (const auto& line : local_matches) {
            output_file_.write(line.data(), static_cast<std::streamsize>(line.size()));
        }
    }

    bar.update(bytes_read, local_matches_count);
}

ParseResult LogParser::run() {
    Timer timer;

    std::uint64_t file_size = fs::file_size(config_.input_path);
    ProgressBar   bar(file_size);

    auto chunks = split_file();

    ThreadPool pool(config_.thread_count);

    auto render_stop = std::atomic<bool>{false};
    auto render_thread = std::thread([&]() {
        while (!render_stop.load(std::memory_order_relaxed)) {
            bar.render(timer.elapsed_sec());
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
        bar.render(timer.elapsed_sec());
        std::cout << "\n";
    });

    std::vector<std::future<void>> futures;
    futures.reserve(chunks.size());

    for (const auto& chunk : chunks) {
        futures.push_back(
            pool.enqueue([this, &chunk, &bar, &timer]() {
                process_chunk(chunk, bar, timer);
            })
        );
    }

    for (auto& f : futures) {
        f.get();
    }

    render_stop.store(true, std::memory_order_relaxed);
    bar.finish();
    render_thread.join();

    timer.stop();
    output_file_.close();

    double elapsed_ms  = timer.elapsed_ms();
    double elapsed_sec = timer.elapsed_sec();
    double speed       = (elapsed_sec > 0.0)
        ? (static_cast<double>(file_size) / (1024.0 * 1024.0)) / elapsed_sec
        : 0.0;

    return ParseResult{
        total_lines_.load(),
        matched_lines_.load(),
        file_size,
        elapsed_ms,
        speed
    };
}