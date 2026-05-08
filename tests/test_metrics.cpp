#include <gtest/gtest.h>
#include <rate_limiter/MetricsCollector.hpp>

using namespace rate_limiter;

// Test that a newly created MetricsCollector has zero metrics
TEST(MetricsCollectorTest, InitialStateIsZero) {
    MetricsCollector collector;
    EXPECT_EQ(collector.getTotalRequests(), 0);
    EXPECT_EQ(collector.getAllowedRequests(), 0);
    EXPECT_EQ(collector.getRejectedRequests(), 0);
    EXPECT_EQ(collector.getAllowanceRate(), 0.0);
}

// Test recording an allowed request
TEST(MetricsCollectorTest, RecordAllowedRequest) {
    MetricsCollector collector;
    collector.recordAllowedRequest();
    EXPECT_EQ(collector.getTotalRequests(), 1);
    EXPECT_EQ(collector.getAllowedRequests(), 1);
    EXPECT_EQ(collector.getRejectedRequests(), 0);
    EXPECT_EQ(collector.getAllowanceRate(), 1.0);
}

// Test recording multiple allowed requests
TEST(MetricsCollectorTest, RecordMultipleAllowedRequests) {
    MetricsCollector collector;
    for (int i = 0; i < 5; i++) {
        collector.recordAllowedRequest();
    }
    EXPECT_EQ(collector.getTotalRequests(), 5);
    EXPECT_EQ(collector.getAllowedRequests(), 5);
    EXPECT_EQ(collector.getRejectedRequests(), 0);
    EXPECT_EQ(collector.getAllowanceRate(), 1.0);
}

// Test recording a rejected request
TEST(MetricsCollectorTest, RecordRejectedRequest) {
    MetricsCollector collector;
    collector.recordRejectedRequest();
    EXPECT_EQ(collector.getTotalRequests(), 1);
    EXPECT_EQ(collector.getAllowedRequests(), 0);
    EXPECT_EQ(collector.getRejectedRequests(), 1);
    EXPECT_EQ(collector.getAllowanceRate(), 0.0);
}

// Test allowance rate calculation with mixed requests
TEST(MetricsCollectorTest, AllowanceRateMixed) {
    MetricsCollector collector;
    collector.recordAllowedRequest();
    collector.recordAllowedRequest();
    collector.recordRejectedRequest();

    EXPECT_EQ(collector.getTotalRequests(), 3);
    EXPECT_EQ(collector.getAllowedRequests(), 2);
    EXPECT_EQ(collector.getRejectedRequests(), 1);
    EXPECT_DOUBLE_EQ(collector.getAllowanceRate(), 2.0 / 3.0);
}

// Test reset functionality
TEST(MetricsCollectorTest, ResetMetrics) {
    MetricsCollector collector;
    collector.recordAllowedRequest();
    collector.recordAllowedRequest();
    collector.recordRejectedRequest();

    EXPECT_EQ(collector.getTotalRequests(), 3);

    collector.reset();

    EXPECT_EQ(collector.getTotalRequests(), 0);
    EXPECT_EQ(collector.getAllowedRequests(), 0);
    EXPECT_EQ(collector.getRejectedRequests(), 0);
    EXPECT_EQ(collector.getAllowanceRate(), 0.0);
}

// Test JSON serialization of metrics
TEST(MetricsCollectorTest, SerializeToJson) {
    MetricsCollector collector;
    collector.recordAllowedRequest();
    collector.recordAllowedRequest();
    collector.recordRejectedRequest();

    auto json = collector.toJson();

    EXPECT_EQ(json["total_requests"], 3);
    EXPECT_EQ(json["allowed_requests"], 2);
    EXPECT_EQ(json["rejected_requests"], 1);
    EXPECT_DOUBLE_EQ(json["allowance_rate"], 2.0 / 3.0);
}

// Test JSON deserialization of metrics
TEST(MetricsCollectorTest, DeserializeFromJson) {
    nlohmann::json json;
    json["total_requests"] = 5;
    json["allowed_requests"] = 3;
    json["rejected_requests"] = 2;
    json["allowance_rate"] = 0.6;

    MetricsCollector collector;
    collector.fromJson(json);

    EXPECT_EQ(collector.getTotalRequests(), 5);
    EXPECT_EQ(collector.getAllowedRequests(), 3);
    EXPECT_EQ(collector.getRejectedRequests(), 2);
    EXPECT_DOUBLE_EQ(collector.getAllowanceRate(), 0.6);
}

// Test round-trip serialization/deserialization
TEST(MetricsCollectorTest, RoundTripSerialization) {
    MetricsCollector collector1;
    for (int i = 0; i < 10; i++) {
        if (i % 3 == 0) {
            collector1.recordRejectedRequest();
        } else {
            collector1.recordAllowedRequest();
        }
    }

    auto json = collector1.toJson();

    MetricsCollector collector2;
    collector2.fromJson(json);

    EXPECT_EQ(collector2.getTotalRequests(), collector1.getTotalRequests());
    EXPECT_EQ(collector2.getAllowedRequests(), collector1.getAllowedRequests());
    EXPECT_EQ(collector2.getRejectedRequests(), collector1.getRejectedRequests());
    EXPECT_DOUBLE_EQ(collector2.getAllowanceRate(), collector1.getAllowanceRate());
}
