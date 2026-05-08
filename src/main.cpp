#include <iostream>
#include <memory>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"

int main() {
    auto metrics = std::make_shared<rate_limiter::MetricsCollector>();
    rate_limiter::RateLimiterManager manager(metrics);

    manager.setAlgorithmFactory([]() {
        return std::make_unique<rate_limiter::TokenBucket>(5.0, 1.0);
    });

    std::cout << "=== Phase 1: Token Bucket Demo ===\n";
    for (int i = 0; i < 8; ++i) {
        bool ok = manager.allowRequest("alice");
        std::cout << "Request " << (i + 1) << ": " << (ok ? "ALLOWED" : "REJECTED") << "\n";
    }

    auto snap = metrics->snapshot();
    std::cout << "\nMetrics:\n"
              << "  Allowed:        " << snap["allowed"]        << "\n"
              << "  Rejected:       " << snap["rejected"]       << "\n"
              << "  Rejection Rate: " << snap["rejection_rate"] << "\n";
    return 0;
}
