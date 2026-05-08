// ─────────────────────────────────────────────
// SlidingWindow
// Tracks timestamps of recent requests in a deque.
// On each allowRequest(), evicts timestamps older
// than windowDuration, then checks count < max.
// Time complexity: O(k) where k = evicted entries.
// Space: O(maxRequests) — bounded deque size.
// Not thread-safe — caller must synchronize.
// ─────────────────────────────────────────────
#pragma once

#include "IRateLimitAlgorithm.hpp"
#include <chrono>
#include <deque>

namespace rate_limiter {

class SlidingWindow : public IRateLimitAlgorithm {
public:
    SlidingWindow(int maxRequests, std::chrono::milliseconds windowDuration);

    [[nodiscard]] bool allowRequest() override;
    void               reset()        override;
    nlohmann::json     serialize()    const override;
    void               deserialize(const nlohmann::json& j) override;

private:
    int maxRequests_;
    std::chrono::milliseconds windowDuration_;
    std::deque<std::chrono::steady_clock::time_point> requestTimestamps_;
};

} // namespace rate_limiter
