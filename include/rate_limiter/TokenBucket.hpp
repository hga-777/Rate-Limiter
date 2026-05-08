// ─────────────────────────────────────────────
// TokenBucket
// Allows up to `capacity` requests, replenishing
// at `refillRatePerSecond` tokens/sec.
// Uses steady_clock — monotonically increasing,
// immune to NTP/DST jumps that break refill math.
// Not thread-safe — caller must synchronize.
// ─────────────────────────────────────────────
#pragma once

#include "IRateLimitAlgorithm.hpp"
#include <chrono>

namespace rate_limiter {

class TokenBucket : public IRateLimitAlgorithm {
public:
    TokenBucket(double capacity, double refillRatePerSecond);

    [[nodiscard]] bool allowRequest() override;
    void               reset()        override;
    nlohmann::json     serialize()    const override;
    void               deserialize(const nlohmann::json& j) override;

private:
    void refill();

    double capacity_;
    double refillRatePerSecond_;
    double tokens_;
    std::chrono::steady_clock::time_point lastRefillTime_;
};

} // namespace rate_limiter
