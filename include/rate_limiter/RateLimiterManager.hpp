// ─────────────────────────────────────────────
// RateLimiterManager
// Owns per-user algorithm instances.
// Thread safety: std::shared_mutex allows many
// concurrent readers (existing users) with an
// exclusive write only on first-seen user insert.
// Algorithms themselves hold no locks.
// ─────────────────────────────────────────────
#pragma once

#include "IRateLimitAlgorithm.hpp"
#include "MetricsCollector.hpp"
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace rate_limiter {

class RateLimiterManager {
public:
    explicit RateLimiterManager(std::shared_ptr<MetricsCollector> metrics);

    bool allowRequest(const std::string& userId);
    void registerUser(const std::string& userId,
                      std::unique_ptr<IRateLimitAlgorithm> algorithm);
    void setAlgorithmFactory(
        std::function<std::unique_ptr<IRateLimitAlgorithm>()> factory);

    nlohmann::json serializeState() const;
    void           deserializeState(const nlohmann::json& state);

private:
    mutable std::shared_mutex                                             mutex_;
    std::unordered_map<std::string, std::unique_ptr<IRateLimitAlgorithm>> limiters_;
    std::shared_ptr<MetricsCollector>                                     metrics_;
    std::function<std::unique_ptr<IRateLimitAlgorithm>()>                algorithmFactory_;
};

} // namespace rate_limiter
