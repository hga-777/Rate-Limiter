// ─────────────────────────────────────────────
// FixedWindow
// Counts requests in a fixed time window.
// Window resets when now >= windowStart + duration.
// Known tradeoff: a burst at the end of window N
// and start of N+1 can pass 2× the limit ("boundary
// burst" problem). SlidingWindow avoids this.
// Not thread-safe — caller must synchronize.
// ─────────────────────────────────────────────
#pragma once

#include "IRateLimitAlgorithm.hpp"
#include <chrono>

namespace rate_limiter {

class FixedWindow : public IRateLimitAlgorithm {
public:
    FixedWindow(int maxRequests, std::chrono::milliseconds windowDuration);

    [[nodiscard]] bool allowRequest() override;
    void               reset()        override;
    nlohmann::json     serialize()    const override;
    void               deserialize(const nlohmann::json& j) override;

private:
    int maxRequests_;
    std::chrono::milliseconds windowDuration_;
    int count_;
    std::chrono::steady_clock::time_point windowStart_;
};

} // namespace rate_limiter
