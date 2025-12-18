#include <chrono>
#include <cstdint>
#include <sched.h>
#include <time.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Before running, jot down the environment (CPU model, WSL version, governor,
// compiler flags) and list the assumptions you’re going to test.

// QUESTION: “Does std::chrono::steady_clock give consistent per-iteration timing
// for a million-iteration no-op loop on my WSL2 setup, or does scheduler/virtualization
// noise dominate?”

double measure_timer_overhead() {
    std::cout << "Using CPU with id: " << sched_getcpu() << " for measure_timer_overhead\n";
    using clock = std::chrono::steady_clock;
    constexpr std::size_t trials = 100000;
    auto start = clock::now();
    for (std::size_t i = 0; i < trials; i++) {
        volatile auto t = clock::now();
        (void)t;
    }
    auto end = clock::now();
    auto total = std::chrono::duration<double>(end - start).count();
    return total / trials;
}

double measure_timer_overhead_sys_clock() {
    std::cout << "Using CPU with id: " << sched_getcpu() << " for measure_timer_overhead_sys_clock\n";
    struct timespec ts;
    volatile double sink = 0.0;
    constexpr std::size_t trials = 100000;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    auto start = ts.tv_sec + ts.tv_nsec * 1e-9;
    for (std::size_t i = 0; i < trials; i++) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        sink += ts.tv_nsec;
    }
    clock_gettime(CLOCK_MONOTONIC, &ts);
    auto end = ts.tv_sec + ts.tv_nsec * 1e-9;
    auto total = end - start;
    return total / trials;
}

double measure_loop(std::uint64_t iterations) {
    std::cout << "Using CPU with id: " << sched_getcpu() << " for measure_loop\n";
    using clock = std::chrono::steady_clock;
    volatile std::uint64_t sink = 0;  // Prevents compiler from optimizing loop away.
    auto start = clock::now();
    for (std::uint64_t i = 0; i < iterations; ++i) {
        // TODO: Add branching logic here to compare control-flow effects.
        sink += i;
    }
    auto end = clock::now();
    return std::chrono::duration<double>(end - start).count();
}

void log_affinity() {
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("Cpus_allowed_list", 0) == 0) {
            std::cout << line << '\n';
            break;
        }
    }
}

void flush_caches() {
    static std::vector<std::uint64_t> junk(64 * 1024 * 1024 / sizeof(std::uint64_t), 0);
    volatile std::uint64_t sink = 0;
    for (auto& v : junk) {
        v ^= 0xdeadbeef;
        sink += v;
    }
    (void)sink;  // keep compiler from dead-storing
}

struct RunRecord {
    int run_index;
    double timer_cost;
    std::uint64_t iterations;
    double loop_time;
    double overhead_to_loop_time_ratio;
    std::string note;
};

int main(int argc, char** argv) {
    const std::string note = (argc > 1) ? argv[1] : "unpinned, warm";
    int cpu_id = -1;
    if (argc > 2) {
        try {
            cpu_id = std::stoi(argv[2]);
        } catch (const std::exception&) {
            std::cerr << "Invalid CPU id '" << argv[2] << "', skipping affinity request\n";
            cpu_id = -1;
        }
    }

    if (cpu_id >= 0) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cpu_id, &set);
        if (sched_setaffinity(0, sizeof(set), &set) == -1) {
            std::perror("sched_setaffinity");
        }
    }

    log_affinity();

    constexpr std::uint64_t iterations = 1'000'000;
    constexpr int runs = 1;
    const std::filesystem::path log_path = "latency_lab/run_log.md";

    std::vector<RunRecord> records;
    records.reserve(runs);

    for (int i = 0; i < runs; ++i) {
        const auto timer_cost = measure_timer_overhead_sys_clock();
        const auto loop_time = measure_loop(iterations);
        const auto overhead_to_loop_time_ratio = timer_cost / loop_time;
        RunRecord rec{
            .run_index = i + 1,
            .timer_cost = timer_cost,
            .iterations = iterations,
            .loop_time = loop_time,
            .overhead_to_loop_time_ratio = overhead_to_loop_time_ratio,
            .note = note,
        };
        records.push_back(rec);
    }

    const bool log_exists = std::filesystem::exists(log_path) && std::filesystem::file_size(log_path) > 0;
    std::ofstream log_file(log_path, std::ios::app);
    if (!log_file) {
        std::cerr << "Failed to open log file: " << log_path << "\n";
        return 1;
    }
    if (!log_exists) {
        log_file << "| Run # | Timer cost (s) | Loop iterations | Loop time (s) | Overhead to Loop time ratio | Conditions / Notes |\n";
        log_file << "|-------|----------------|-----------------|---------------|-----------------------------|--------------------|\n";
    }

        std::cout << "| Run # | Timer cost (s) | Loop iterations | Loop time (s) | Overhead to Loop time ratio | Conditions / Notes |\n";
        std::cout << "|-------|----------------|-----------------|---------------|-----------------------------|--------------------|\n";
    std::cout << std::fixed << std::setprecision(9);
    for (const auto& rec : records) {
        std::cout << "| " << rec.run_index << " | "
                  << rec.timer_cost << " | "
                  << rec.iterations << " | "
                  << rec.loop_time << " | "
                  << rec.overhead_to_loop_time_ratio << " | "
                  << rec.note << " |\n";
        log_file << "| " << rec.run_index << " | "
                 << rec.timer_cost << " | "
                 << rec.iterations << " | "
                 << rec.loop_time << " | "
                 << rec.overhead_to_loop_time_ratio << " | "
                 << rec.note << " |\n";
    }

    return 0;
}
