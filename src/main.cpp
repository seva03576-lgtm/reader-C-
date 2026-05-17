#include "log_parser.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <cstdlib>
#include <thread>

namespace fs = std::filesystem;

static void print_banner() {
    std::cout
        << "\n\033[36m╔══════════════════════════════════════════════════╗\033[0m\n"
        << "\033[36m║\033[0m  \033[97m  High-Performance Multi-Threaded Log Parser\033[0m  \033[36m║\033[0m\n"
        << "\033[36m╚══════════════════════════════════════════════════╝\033[0m\n\n";
}

static void print_usage(const char* program) {
    std::cerr
        << "\033[93mUsage:\033[0m\n"
        << "  " << program << " <log_file> <keyword> [output_file] [threads] [chunk_mb]\n\n"
        << "\033[93mArguments:\033[0m\n"
        << "  log_file     Path to the input log file\n"
        << "  keyword      Keyword to search for (e.g. ERROR, WARNING)\n"
        << "  output_file  Path to output file (default: filtered_output.log)\n"
        << "  threads      Number of threads (default: hardware concurrency)\n"
        << "  chunk_mb     Chunk size in MB (default: 32)\n\n"
        << "\033[93mExample:\033[0m\n"
        << "  " << program << " server.log ERROR filtered_errors.log 8 64\n\n";
}

static void print_result(const ParseResult& result, const fs::path& output_path) {
    double file_mb = static_cast<double>(result.total_bytes) / (1024.0 * 1024.0);

    std::cout
        << "\n\033[36m┌─────────────────────────────────────────────────┐\033[0m\n"
        << "\033[36m│\033[0m              \033[97m  Parse Results\033[0m                    \033[36m│\033[0m\n"
        << "\033[36m├─────────────────────────────────────────────────┤\033[0m\n"
        << "\033[36m│\033[0m  File size     : \033[97m"
            << std::fixed << std::setprecision(2) << file_mb << " MB\033[0m\n"
        << "\033[36m│\033[0m  Total lines   : \033[97m" << result.total_lines  << "\033[0m\n"
        << "\033[36m│\033[0m  Matched lines : \033[92m" << result.matched_lines << "\033[0m\n"
        << "\033[36m│\033[0m  Speed         : \033[93m"
            << std::fixed << std::setprecision(2) << result.speed_mb_sec << " MB/s\033[0m\n"
        << "\033[36m│\033[0m  Elapsed time  : \033[93m"
            << std::fixed << std::setprecision(3) << result.elapsed_ms << " ms\033[0m\n"
        << "\033[36m│\033[0m  Output file   : \033[94m" << output_path.string() << "\033[0m\n"
        << "\033[36m└─────────────────────────────────────────────────┘\033[0m\n\n";
}

int main(int argc, char* argv[]) {
    print_banner();

    if (argc < 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    try {
        ParseConfig config;
        config.input_path   = fs::path(argv[1]);
        config.keyword      = argv[2];
        config.output_path  = (argc >= 4) ? fs::path(argv[3]) : fs::path("filtered_output.log");
        config.thread_count = (argc >= 5) ? static_cast<std::size_t>(std::stoul(argv[4])) : 0;
        config.chunk_size   = (argc >= 6)
            ? static_cast<std::size_t>(std::stoul(argv[5])) * 1024 * 1024
            : 0;

        std::size_t hw = std::thread::hardware_concurrency();
        if (hw == 0) hw = 4;
        std::size_t threads_used = (config.thread_count > 0) ? config.thread_count : hw;

        std::cout
            << "\033[90m  Input  : \033[0m\033[97m" << config.input_path.string()  << "\033[0m\n"
            << "\033[90m  Keyword: \033[0m\033[91m" << config.keyword               << "\033[0m\n"
            << "\033[90m  Output : \033[0m\033[97m" << config.output_path.string() << "\033[0m\n"
            << "\033[90m  Threads: \033[0m\033[97m" << threads_used                << "\033[0m\n\n";

        LogParser parser(config);
        ParseResult result = parser.run();

        print_result(result, config.output_path);

    } catch (const std::exception& ex) {
        std::cerr << "\n\033[91m✗ Error: " << ex.what() << "\033[0m\n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}