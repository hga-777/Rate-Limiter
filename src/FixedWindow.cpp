// stub — full implementation in Task 8
#include "rate_limiter/FixedWindow.hpp"

namespace rate_limiter {

FixedWindow::FixedWindow(int maxRequests, std::chrono::milliseconds windowDuration)
    : maxRequests_(maxRequests), windowDuration_(windowDuration),
      count_(0), windowStart_(std::chrono::steady_clock::now()) {}

bool FixedWindow::allowRequest() { return false; }
void FixedWindow::reset()        { count_ = 0; windowStart_ = std::chrono::steady_clock::now(); }

nlohmann::json FixedWindow::serialize() const {
    return {{"type", "fixed_window"},
            {"max_requests", maxRequests_},
            {"window_ms", windowDuration_.count()}};
}

void FixedWindow::deserialize(const nlohmann::json&) {}

} // namespace rate_limiter
