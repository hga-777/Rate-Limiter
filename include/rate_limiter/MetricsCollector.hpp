// ─────────────────────────────────────────────
// MetricsCollector
// Lock-free counters using std::atomic.
// fetch_add compiles to a single LOCK XADD
// instruction — no mutex, no contention.
// ─────────────────────────────────────────────
#pragma once

#include <atomic>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace rate_limiter {

class MetricsCollector {
public:
    void recordAllowed();
    void recordRejected();
    nlohmann::json snapshot() const;

private:
    std::atomic<uint64_t> allowed_{0};
    std::atomic<uint64_t> rejected_{0};
};

} // namespace rate_limiter
