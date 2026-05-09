#include <iostream>
#include <memory>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"
#include "rate_limiter/SlidingWindow.hpp"
#include "rate_limiter/FixedWindow.hpp"

using namespace rate_limiter;
using namespace std::chrono_literals;

static void runDemo(RateLimiterManager& mgr, const std::string& userId,
                    const std::string& label) {
    std::cout << "\n--- " << label << " ---\n";
    for (int i = 0; i < 5; ++i) {
        bool ok = mgr.allowRequest(userId);
        std::cout << "Request " << (i + 1) << ": " << (ok ? "ALLOWED" : "REJECTED") << "\n";
    }
}

int main() {
    auto metrics = std::make_shared<MetricsCollector>();
    RateLimiterManager manager(metrics);

    // Phase 1: Token Bucket
    manager.setAlgorithmFactory([]() {
        return std::make_unique<TokenBucket>(3.0, 1.0);
    });
    runDemo(manager, "alice", "Token Bucket (capacity=3)");

    // Phase 2: Hot-swap to Sliding Window for new users
    manager.setAlgorithmFactory([]() {
        return std::make_unique<SlidingWindow>(3, 1000ms);
    });
    manager.registerUser("bob", std::make_unique<SlidingWindow>(3, 1000ms));
    runDemo(manager, "bob", "Sliding Window (max=3, window=1s) — bob");

    // Phase 2: Fixed Window
    manager.setAlgorithmFactory([]() {
        return std::make_unique<FixedWindow>(3, 1000ms);
    });
    manager.registerUser("carol", std::make_unique<FixedWindow>(3, 1000ms));
    runDemo(manager, "carol", "Fixed Window (max=3, window=1s) — carol");

    return 0;
}
