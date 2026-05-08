// ─────────────────────────────────────────────
// MetricsCollector
// Tracks allowed/rejected requests and
// computes allowance rate. Not thread-safe.
// ─────────────────────────────────────────────
#pragma once

#include <nlohmann/json.hpp>

namespace rate_limiter {

class MetricsCollector {
private:
    uint64_t totalRequests = 0;
    uint64_t allowedRequests = 0;
    uint64_t rejectedRequests = 0;

public:
    MetricsCollector() = default;

    void recordAllowedRequest();
    void recordRejectedRequest();

    [[nodiscard]] uint64_t getTotalRequests() const;
    [[nodiscard]] uint64_t getAllowedRequests() const;
    [[nodiscard]] uint64_t getRejectedRequests() const;
    [[nodiscard]] double getAllowanceRate() const;

    void reset();

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

} // namespace rate_limiter
