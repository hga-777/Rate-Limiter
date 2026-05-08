#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/TokenBucket.hpp"
#include "rate_limiter/SlidingWindow.hpp"
#include "rate_limiter/FixedWindow.hpp"
#include <stdexcept>

namespace rate_limiter {

RateLimiterManager::RateLimiterManager(std::shared_ptr<MetricsCollector> metrics)
    : metrics_(std::move(metrics))
{}

bool RateLimiterManager::allowRequest(const std::string& userId) {
    if (!algorithmFactory_) {
        throw std::runtime_error("Algorithm factory not set — call setAlgorithmFactory first");
    }

    // Fast path: shared lock — multiple threads can read concurrently.
    {
        std::shared_lock<std::shared_mutex> readLock(mutex_);
        auto it = limiters_.find(userId);
        if (it != limiters_.end()) {
            bool allowed = it->second->allowRequest();
            allowed ? metrics_->recordAllowed() : metrics_->recordRejected();
            return allowed;
        }
    }

    // Slow path: exclusive write lock to insert new user.
    // Double-check after acquiring write lock — another thread may have
    // inserted this user between our two lock acquisitions.
    {
        std::unique_lock<std::shared_mutex> writeLock(mutex_);
        auto [it, inserted] = limiters_.emplace(userId, nullptr);
        if (inserted) {
            it->second = algorithmFactory_();
        }
        bool allowed = it->second->allowRequest();
        allowed ? metrics_->recordAllowed() : metrics_->recordRejected();
        return allowed;
    }
}

void RateLimiterManager::registerUser(const std::string& userId,
                                      std::unique_ptr<IRateLimitAlgorithm> algorithm) {
    std::unique_lock<std::shared_mutex> writeLock(mutex_);
    limiters_[userId] = std::move(algorithm);
}

void RateLimiterManager::setAlgorithmFactory(
    std::function<std::unique_ptr<IRateLimitAlgorithm>()> factory) {
    algorithmFactory_ = std::move(factory);
}

nlohmann::json RateLimiterManager::serializeState() const {
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    nlohmann::json state = nlohmann::json::object();
    for (const auto& [userId, algo] : limiters_) {
        state[userId] = algo->serialize();
    }
    return state;
}

void RateLimiterManager::deserializeState(const nlohmann::json& state) {
    std::unique_lock<std::shared_mutex> writeLock(mutex_);
    limiters_.clear();
    for (const auto& [userId, algoState] : state.items()) {
        std::string type = algoState.at("type").get<std::string>();
        std::unique_ptr<IRateLimitAlgorithm> algo;
        if (type == "token_bucket") {
            auto tb = std::make_unique<TokenBucket>(
                algoState.at("capacity").get<double>(),
                algoState.at("refill_rate").get<double>());
            tb->deserialize(algoState);
            algo = std::move(tb);
        } else if (type == "sliding_window") {
            auto sw = std::make_unique<SlidingWindow>(
                algoState.at("max_requests").get<int>(),
                std::chrono::milliseconds{algoState.at("window_ms").get<int64_t>()});
            sw->deserialize(algoState);
            algo = std::move(sw);
        } else if (type == "fixed_window") {
            auto fw = std::make_unique<FixedWindow>(
                algoState.at("max_requests").get<int>(),
                std::chrono::milliseconds{algoState.at("window_ms").get<int64_t>()});
            fw->deserialize(algoState);
            algo = std::move(fw);
        }
        if (algo) limiters_.emplace(userId, std::move(algo));
    }
}

} // namespace rate_limiter
