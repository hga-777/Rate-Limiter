#include <gtest/gtest.h>
#include "rate_limiter/MetricsCollector.hpp"

using namespace rate_limiter;

TEST(MetricsCollectorTest, InitialCountsAreZero) {
    MetricsCollector m;
    auto snap = m.snapshot();
    EXPECT_EQ(snap["allowed"].get<uint64_t>(),  0u);
    EXPECT_EQ(snap["rejected"].get<uint64_t>(), 0u);
    EXPECT_EQ(snap["total"].get<uint64_t>(),    0u);
}

TEST(MetricsCollectorTest, RecordAllowedIncrements) {
    MetricsCollector m;
    m.recordAllowed();
    m.recordAllowed();
    auto snap = m.snapshot();
    EXPECT_EQ(snap["allowed"].get<uint64_t>(), 2u);
    EXPECT_EQ(snap["total"].get<uint64_t>(),   2u);
}

TEST(MetricsCollectorTest, RecordRejectedIncrements) {
    MetricsCollector m;
    m.recordRejected();
    auto snap = m.snapshot();
    EXPECT_EQ(snap["rejected"].get<uint64_t>(), 1u);
    EXPECT_EQ(snap["total"].get<uint64_t>(),    1u);
}

TEST(MetricsCollectorTest, RejectionRateIsCalculated) {
    MetricsCollector m;
    m.recordAllowed();
    m.recordRejected();
    auto snap = m.snapshot();
    EXPECT_EQ(snap["rejection_rate"].get<std::string>(), "50.00%");
}
