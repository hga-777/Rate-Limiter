// ─────────────────────────────────────────────
// Multi-threaded simulation
// Spawns NUM_THREADS threads, each sending
// REQUESTS_PER_THREAD requests across NUM_USERS
// shared users. Demonstrates thread safety of
// RateLimiterManager under real contention.
//
// Scalability note: the coarse shared_mutex in
// RateLimiterManager is the bottleneck here.
// Under extreme concurrency, per-user mutexes
// (stored alongside each algorithm instance)
// would eliminate cross-user contention.
// ─────────────────────────────────────────────
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iomanip>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"

using namespace rate_limiter;
using namespace std::chrono_literals;

int main() {
    constexpr int NUM_THREADS          = 20;
    constexpr int REQUESTS_PER_THREAD  = 500;
    constexpr int NUM_USERS            = 5;
    constexpr double BUCKET_CAPACITY   = 50.0;
    constexpr double REFILL_RATE       = 10.0;

    auto metrics = std::make_shared<MetricsCollector>();
    RateLimiterManager manager(metrics);
    manager.setAlgorithmFactory([=]() {
        return std::make_unique<TokenBucket>(BUCKET_CAPACITY, REFILL_RATE);
    });

    std::cout << "Starting simulation:\n"
              << "  Threads:           " << NUM_THREADS << "\n"
              << "  Requests/thread:   " << REQUESTS_PER_THREAD << "\n"
              << "  Total requests:    " << (NUM_THREADS * REQUESTS_PER_THREAD) << "\n"
              << "  Users:             " << NUM_USERS << "\n"
              << "  Bucket capacity:   " << BUCKET_CAPACITY << "\n"
              << "  Refill rate:       " << REFILL_RATE << "/sec\n\n";

    auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (int r = 0; r < REQUESTS_PER_THREAD; ++r) {
                std::string userId = "user_" + std::to_string(t % NUM_USERS);
                manager.allowRequest(userId);
            }
        });
    }
    for (auto& th : threads) th.join();

    auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();

    auto snap = metrics->snapshot();
    uint64_t total = snap["total"].get<uint64_t>();

    std::cout << "Results:\n"
              << "  Allowed:        " << snap["allowed"]                           << "\n"
              << "  Rejected:       " << snap["rejected"]                          << "\n"
              << "  Total:          " << total                                     << "\n"
              << "  Rejection Rate: " << snap["rejection_rate"].get<std::string>() << "\n"
              << "  Elapsed:        " << std::fixed << std::setprecision(3)
              << elapsed << "s\n"
              << "  Throughput:     " << std::fixed << std::setprecision(0)
              << (total / elapsed) << " req/s\n";
    return 0;
}
