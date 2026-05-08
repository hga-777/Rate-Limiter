#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "rate_limiter/TokenBucket.hpp"

using namespace rate_limiter;
using namespace std::chrono_literals;

TEST(TokenBucketTest, AllowsRequestsUpToCapacity) {
    TokenBucket tb(5.0, 1.0);
    EXPECT_TRUE(tb.allowRequest());
    EXPECT_TRUE(tb.allowRequest());
    EXPECT_TRUE(tb.allowRequest());
    EXPECT_TRUE(tb.allowRequest());
    EXPECT_TRUE(tb.allowRequest());
    EXPECT_FALSE(tb.allowRequest()); // bucket empty
}

TEST(TokenBucketTest, RefillsTokensOverTime) {
    TokenBucket tb(1.0, 20.0); // capacity=1, refill 20/sec
    EXPECT_TRUE(tb.allowRequest());   // consume only token
    EXPECT_FALSE(tb.allowRequest());  // empty
    std::this_thread::sleep_for(100ms); // wait ~2 tokens worth at 20/sec
    EXPECT_TRUE(tb.allowRequest());   // refilled
}

TEST(TokenBucketTest, ResetRestoresFullBucket) {
    TokenBucket tb(3.0, 1.0);
    tb.allowRequest();
    tb.allowRequest();
    tb.allowRequest();
    EXPECT_FALSE(tb.allowRequest());
    tb.reset();
    EXPECT_TRUE(tb.allowRequest());
}

TEST(TokenBucketTest, SerializePreservesTokenCount) {
    TokenBucket tb(10.0, 5.0);
    tb.allowRequest();
    tb.allowRequest(); // 8 tokens remain
    auto j = tb.serialize();

    EXPECT_EQ(j["type"].get<std::string>(), "token_bucket");
    EXPECT_NEAR(j["tokens"].get<double>(), 8.0, 0.01);
}

TEST(TokenBucketTest, DeserializeRestoresState) {
    TokenBucket tb(10.0, 5.0);
    tb.allowRequest();
    tb.allowRequest(); // 8 tokens remain
    auto j = tb.serialize();

    TokenBucket tb2(1.0, 1.0); // different initial config
    tb2.deserialize(j);

    int allowed = 0;
    for (int i = 0; i < 10; ++i) {
        if (tb2.allowRequest()) ++allowed;
    }
    EXPECT_EQ(allowed, 8);
}
