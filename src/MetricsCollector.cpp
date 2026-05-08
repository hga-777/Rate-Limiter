#include <rate_limiter/MetricsCollector.hpp>

namespace rate_limiter {

void MetricsCollector::recordAllowedRequest() {
    totalRequests++;
    allowedRequests++;
}

void MetricsCollector::recordRejectedRequest() {
    totalRequests++;
    rejectedRequests++;
}

uint64_t MetricsCollector::getTotalRequests() const {
    return totalRequests;
}

uint64_t MetricsCollector::getAllowedRequests() const {
    return allowedRequests;
}

uint64_t MetricsCollector::getRejectedRequests() const {
    return rejectedRequests;
}

double MetricsCollector::getAllowanceRate() const {
    if (totalRequests == 0) {
        return 0.0;
    }
    return static_cast<double>(allowedRequests) / static_cast<double>(totalRequests);
}

void MetricsCollector::reset() {
    totalRequests = 0;
    allowedRequests = 0;
    rejectedRequests = 0;
}

nlohmann::json MetricsCollector::toJson() const {
    nlohmann::json j;
    j["total_requests"] = totalRequests;
    j["allowed_requests"] = allowedRequests;
    j["rejected_requests"] = rejectedRequests;
    j["allowance_rate"] = getAllowanceRate();
    return j;
}

void MetricsCollector::fromJson(const nlohmann::json& j) {
    totalRequests = j["total_requests"].get<uint64_t>();
    allowedRequests = j["allowed_requests"].get<uint64_t>();
    rejectedRequests = j["rejected_requests"].get<uint64_t>();
    // allowance_rate is computed, not stored
}

} // namespace rate_limiter
