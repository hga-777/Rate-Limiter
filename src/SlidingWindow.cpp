// ─────────────────────────────────────────────
// SlidingWindow
// Tracks timestamps of recent requests in a deque.
// On each call, evicts timestamps older than the
// window, then checks if count < maxRequests.
// Time complexity: O(k) where k = evicted entries.
// Space: O(maxRequests) — bounded queue size.
// ─────────────────────────────────────────────
#include "rate_limiter/SlidingWindow.hpp"
#include <stdexcept>

namespace rate_limiter {

SlidingWindow::SlidingWindow(int maxRequests, std::chrono::milliseconds windowDuration)
    : maxRequests_(maxRequests)
    , windowDuration_(windowDuration)
{}

bool SlidingWindow::allowRequest() {
    auto now = std::chrono::steady_clock::now();

    while (!requestTimestamps_.empty() &&
           now - requestTimestamps_.front() > windowDuration_) {
        requestTimestamps_.pop_front();
    }

    if (static_cast<int>(requestTimestamps_.size()) < maxRequests_) {
        requestTimestamps_.push_back(now);
        return true;
    }
    return false;
}

void SlidingWindow::reset() {
    requestTimestamps_.clear();
}

nlohmann::json SlidingWindow::serialize() const {
    return {
        {"type",         "sliding_window"},
        {"max_requests", maxRequests_},
        {"window_ms",    windowDuration_.count()}
    };
    // Timestamps are steady_clock-based (process-local) and cannot be
    // meaningfully restored across restarts — we serialize only config.
    // On restore, the window starts fresh, which is conservative and correct.
}

void SlidingWindow::deserialize(const nlohmann::json& j) {
    int  maxReq = j.at("max_requests").get<int>();
    auto winMs  = j.at("window_ms").get<int64_t>();
    if (maxReq <= 0 || winMs <= 0)
        throw std::invalid_argument("SlidingWindow::deserialize: invalid config");
    maxRequests_    = maxReq;
    windowDuration_ = std::chrono::milliseconds{winMs};
    requestTimestamps_.clear();
}

} // namespace rate_limiter
