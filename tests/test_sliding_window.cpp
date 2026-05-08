#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "rate_limiter/SlidingWindow.hpp"

using namespace rate_limiter;
using namespace std::chrono_literals;

TEST(SlidingWindowTest, AllowsUpToMaxRequestsInWindow) {
    SlidingWindow sw(3, 1000ms);
    EXPECT_TRUE(sw.allowRequest());
    EXPECT_TRUE(sw.allowRequest());
    EXPECT_TRUE(sw.allowRequest());
    EXPECT_FALSE(sw.allowRequest()); // 4th in same window — rejected
}

TEST(SlidingWindowTest, AllowsAfterWindowExpires) {
    SlidingWindow sw(2, 100ms);
    EXPECT_TRUE(sw.allowRequest());
    EXPECT_TRUE(sw.allowRequest());
    EXPECT_FALSE(sw.allowRequest());
    std::this_thread::sleep_for(110ms); // window expires
    EXPECT_TRUE(sw.allowRequest());     // oldest request slid out
}

TEST(SlidingWindowTest, ResetClearsAllTimestamps) {
    SlidingWindow sw(2, 1000ms);
    EXPECT_TRUE(sw.allowRequest());
    EXPECT_TRUE(sw.allowRequest());
    EXPECT_FALSE(sw.allowRequest());
    sw.reset();
    EXPECT_TRUE(sw.allowRequest());
}

TEST(SlidingWindowTest, SerializePreservesConfig) {
    SlidingWindow sw(5, 500ms);
    auto j = sw.serialize();
    EXPECT_EQ(j["type"].get<std::string>(), "sliding_window");
    EXPECT_EQ(j["max_requests"].get<int>(), 5);
    EXPECT_EQ(j["window_ms"].get<int64_t>(), 500);
}

TEST(SlidingWindowTest, DeserializeRestoresConfig) {
    SlidingWindow sw(5, 500ms);
    auto j = sw.serialize();

    SlidingWindow sw2(1, 1ms); // different initial values
    sw2.deserialize(j);
    // After deserialize, should allow 5 requests in 500ms window
    int allowed = 0;
    for (int i = 0; i < 7; ++i) {
        if (sw2.allowRequest()) ++allowed;
    }
    EXPECT_EQ(allowed, 5);
}
