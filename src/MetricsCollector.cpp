#include "rate_limiter/MetricsCollector.hpp"
#include <sstream>
#include <iomanip>

namespace rate_limiter {

void MetricsCollector::recordAllowed() {
    allowed_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::recordRejected() {
    rejected_.fetch_add(1, std::memory_order_relaxed);
}

nlohmann::json MetricsCollector::snapshot() const {
    uint64_t allowed  = allowed_.load(std::memory_order_relaxed);
    uint64_t rejected = rejected_.load(std::memory_order_relaxed);
    uint64_t total    = allowed + rejected;
    double   rate     = (total > 0) ? (100.0 * rejected / total) : 0.0;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << rate << "%";

    return {
        {"allowed",        allowed},
        {"rejected",       rejected},
        {"total",          total},
        {"rejection_rate", oss.str()}
    };
}

} // namespace rate_limiter
