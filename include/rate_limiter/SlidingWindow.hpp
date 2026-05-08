// ─────────────────────────────────────────────
// SlidingWindow — stub (will be implemented in Task 7)
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
    int                          maxRequests_;
    std::chrono::milliseconds    windowDuration_;
    std::deque<std::chrono::steady_clock::time_point> timestamps_;
};

} // namespace rate_limiter
