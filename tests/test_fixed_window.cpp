#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "rate_limiter/FixedWindow.hpp"

using namespace rate_limiter;
using namespace std::chrono_literals;

TEST(FixedWindowTest, AllowsUpToMaxRequestsInWindow) {
    FixedWindow fw(3, 1000ms);
    EXPECT_TRUE(fw.allowRequest());
    EXPECT_TRUE(fw.allowRequest());
    EXPECT_TRUE(fw.allowRequest());
    EXPECT_FALSE(fw.allowRequest());
}

TEST(FixedWindowTest, ResetsCounterAfterWindowExpires) {
    FixedWindow fw(2, 100ms);
    EXPECT_TRUE(fw.allowRequest());
    EXPECT_TRUE(fw.allowRequest());
    EXPECT_FALSE(fw.allowRequest());
    std::this_thread::sleep_for(110ms); // new window
    EXPECT_TRUE(fw.allowRequest());     // counter reset
}

TEST(FixedWindowTest, ResetRestoresFullWindow) {
    FixedWindow fw(2, 1000ms);
    EXPECT_TRUE(fw.allowRequest());
    EXPECT_TRUE(fw.allowRequest());
    EXPECT_FALSE(fw.allowRequest());
    fw.reset();
    EXPECT_TRUE(fw.allowRequest());
}

TEST(FixedWindowTest, SerializePreservesCountAndConfig) {
    FixedWindow fw(5, 500ms);
    EXPECT_TRUE(fw.allowRequest());
    EXPECT_TRUE(fw.allowRequest());
    auto j = fw.serialize();
    EXPECT_EQ(j["type"].get<std::string>(), "fixed_window");
    EXPECT_EQ(j["max_requests"].get<int>(), 5);
    EXPECT_EQ(j["count"].get<int>(), 2);
}

TEST(FixedWindowTest, DeserializeRestoresCount) {
    FixedWindow fw(5, 1000ms);
    EXPECT_TRUE(fw.allowRequest());
    EXPECT_TRUE(fw.allowRequest()); // count = 2
    auto j = fw.serialize();

    FixedWindow fw2(1, 1ms);
    fw2.deserialize(j);
    // 3 requests remain in this window
    int allowed = 0;
    for (int i = 0; i < 5; ++i) {
        if (fw2.allowRequest()) ++allowed;
    }
    EXPECT_EQ(allowed, 3);
}
