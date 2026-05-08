#include "rate_limiter/TokenBucket.hpp"
#include <algorithm>

namespace rate_limiter {

TokenBucket::TokenBucket(double capacity, double refillRatePerSecond)
    : capacity_(capacity)
    , refillRatePerSecond_(refillRatePerSecond)
    , tokens_(capacity)
    , lastRefillTime_(std::chrono::steady_clock::now())
{}

void TokenBucket::refill() {
    auto   now     = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - lastRefillTime_).count();
    tokens_        = std::min(capacity_, tokens_ + elapsed * refillRatePerSecond_);
    lastRefillTime_ = now;
}

bool TokenBucket::allowRequest() {
    refill();
    if (tokens_ >= 1.0) {
        tokens_ -= 1.0;
        return true;
    }
    return false;
}

void TokenBucket::reset() {
    tokens_         = capacity_;
    lastRefillTime_ = std::chrono::steady_clock::now();
}

nlohmann::json TokenBucket::serialize() const {
    return {
        {"type",        "token_bucket"},
        {"capacity",    capacity_},
        {"refill_rate", refillRatePerSecond_},
        {"tokens",      tokens_}
    };
}

void TokenBucket::deserialize(const nlohmann::json& j) {
    double cap  = j.at("capacity").get<double>();
    double rate = j.at("refill_rate").get<double>();
    double toks = j.at("tokens").get<double>();
    if (cap <= 0.0 || rate < 0.0 || toks < 0.0 || toks > cap)
        throw std::invalid_argument("TokenBucket::deserialize: invalid state in JSON");
    capacity_            = cap;
    refillRatePerSecond_ = rate;
    tokens_              = toks;
    // Reset to now — steady_clock is process-local and cannot be
    // meaningfully restored across restarts. Conservative and correct.
    lastRefillTime_      = std::chrono::steady_clock::now();
}

} // namespace rate_limiter
