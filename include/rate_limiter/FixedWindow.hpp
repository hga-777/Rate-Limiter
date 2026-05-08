// ─────────────────────────────────────────────
// FixedWindow — stub (will be implemented in Task 8)
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
    int                       maxRequests_;
    std::chrono::milliseconds windowDuration_;
    int                       count_{0};
    std::chrono::steady_clock::time_point windowStart_;
};

} // namespace rate_limiter
