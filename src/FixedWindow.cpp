// ─────────────────────────────────────────────
// FixedWindow
// Counts requests in a fixed time window.
// Window resets when current time >= windowStart + duration.
// Known tradeoff vs SlidingWindow: a burst at the
// end of window N and start of window N+1 can pass
// 2× the limit. This is the "boundary burst" problem.
// ─────────────────────────────────────────────
#include "rate_limiter/FixedWindow.hpp"
#include <stdexcept>

namespace rate_limiter {

FixedWindow::FixedWindow(int maxRequests, std::chrono::milliseconds windowDuration)
    : maxRequests_(maxRequests)
    , windowDuration_(windowDuration)
    , count_(0)
    , windowStart_(std::chrono::steady_clock::now())
{
    if (maxRequests <= 0 || windowDuration.count() <= 0)
        throw std::invalid_argument("FixedWindow: maxRequests and windowDuration must be positive");
}

bool FixedWindow::allowRequest() {
    auto now = std::chrono::steady_clock::now();
    if (now - windowStart_ >= windowDuration_) {
        count_       = 0;
        windowStart_ = now;
    }
    if (count_ < maxRequests_) {
        ++count_;
        return true;
    }
    return false;
}

void FixedWindow::reset() {
    count_       = 0;
    windowStart_ = std::chrono::steady_clock::now();
}

nlohmann::json FixedWindow::serialize() const {
    return {
        {"type",         "fixed_window"},
        {"max_requests", maxRequests_},
        {"window_ms",    windowDuration_.count()},
        {"count",        count_}
    };
}

void FixedWindow::deserialize(const nlohmann::json& j) {
    int  maxReq = j.at("max_requests").get<int>();
    auto winMs  = j.at("window_ms").get<int64_t>();
    int  cnt    = j.at("count").get<int>();
    if (maxReq <= 0 || winMs <= 0 || cnt < 0 || cnt > maxReq)
        throw std::invalid_argument("FixedWindow::deserialize: invalid state");
    maxRequests_    = maxReq;
    windowDuration_ = std::chrono::milliseconds{winMs};
    count_          = cnt;
    windowStart_    = std::chrono::steady_clock::now();
}

} // namespace rate_limiter
