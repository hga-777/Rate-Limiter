// ─────────────────────────────────────────────
// IRateLimitAlgorithm
// Strategy interface — each implementation is
// stateful for a single user, contains no locks.
// Callers must guarantee single-threaded access.
// ─────────────────────────────────────────────
#pragma once

#include <nlohmann/json.hpp>

namespace rate_limiter {

class IRateLimitAlgorithm {
public:
    virtual ~IRateLimitAlgorithm() = default;

    virtual bool allowRequest() = 0;
    virtual void reset() = 0;

    virtual nlohmann::json serialize() const = 0;
    virtual void deserialize(const nlohmann::json& j) = 0;
};

} // namespace rate_limiter
