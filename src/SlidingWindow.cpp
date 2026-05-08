// stub — full implementation in Task 7
#include "rate_limiter/SlidingWindow.hpp"

namespace rate_limiter {

SlidingWindow::SlidingWindow(int maxRequests, std::chrono::milliseconds windowDuration)
    : maxRequests_(maxRequests), windowDuration_(windowDuration) {}

bool SlidingWindow::allowRequest() { return false; }
void SlidingWindow::reset()        { timestamps_.clear(); }

nlohmann::json SlidingWindow::serialize() const {
    return {{"type", "sliding_window"},
            {"max_requests", maxRequests_},
            {"window_ms", windowDuration_.count()}};
}

void SlidingWindow::deserialize(const nlohmann::json&) {}

} // namespace rate_limiter
