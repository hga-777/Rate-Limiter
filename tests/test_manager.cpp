#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"

using namespace rate_limiter;

static std::shared_ptr<RateLimiterManager> makeManager(double capacity = 5.0,
                                                        double refill  = 1.0) {
    auto metrics = std::make_shared<MetricsCollector>();
    auto mgr     = std::make_shared<RateLimiterManager>(metrics);
    mgr->setAlgorithmFactory([capacity, refill]() {
        return std::make_unique<TokenBucket>(capacity, refill);
    });
    return mgr;
}

TEST(RateLimiterManagerTest, AutoRegistersNewUsers) {
    auto mgr = makeManager();
    EXPECT_TRUE(mgr->allowRequest("alice"));
}

TEST(RateLimiterManagerTest, EnforcesLimitPerUser) {
    auto mgr = makeManager(/*capacity=*/3.0, /*refill=*/0.0);
    EXPECT_TRUE(mgr->allowRequest("bob"));
    EXPECT_TRUE(mgr->allowRequest("bob"));
    EXPECT_TRUE(mgr->allowRequest("bob"));
    EXPECT_FALSE(mgr->allowRequest("bob"));
}

TEST(RateLimiterManagerTest, UsersAreIndependent) {
    auto mgr = makeManager(/*capacity=*/1.0, /*refill=*/0.0);
    EXPECT_TRUE(mgr->allowRequest("alice"));
    EXPECT_FALSE(mgr->allowRequest("alice"));
    EXPECT_TRUE(mgr->allowRequest("bob")); // bob has a fresh bucket
}

TEST(RateLimiterManagerTest, MetricsAreUpdated) {
    auto metrics = std::make_shared<MetricsCollector>();
    auto mgr     = std::make_shared<RateLimiterManager>(metrics);
    mgr->setAlgorithmFactory([]() {
        return std::make_unique<TokenBucket>(1.0, 0.0);
    });
    mgr->allowRequest("carol"); // allowed
    mgr->allowRequest("carol"); // rejected
    auto snap = metrics->snapshot();
    EXPECT_EQ(snap["allowed"].get<uint64_t>(),  1u);
    EXPECT_EQ(snap["rejected"].get<uint64_t>(), 1u);
}

TEST(RateLimiterManagerTest, ThreadSafeUnderConcurrentAccess) {
    auto mgr = makeManager(/*capacity=*/1000.0, /*refill=*/0.0);
    std::atomic<int> allowed{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&, t]() {
            for (int r = 0; r < 50; ++r) {
                if (mgr->allowRequest("user_" + std::to_string(t % 3)))
                    ++allowed;
            }
        });
    }
    for (auto& th : threads) th.join();
    // 3 users × 1000 capacity = 3000 possible allowed; 10 threads × 50 = 500 total
    EXPECT_EQ(allowed.load(), 500); // all allowed — well under capacity
}
