#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "rate_limiter/PersistenceManager.hpp"
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"

using namespace rate_limiter;
namespace fs = std::filesystem;

static const fs::path kTestFile = fs::temp_directory_path() / "rl_test_state.json";

static std::shared_ptr<RateLimiterManager> makeManager() {
    auto metrics = std::make_shared<MetricsCollector>();
    auto mgr     = std::make_shared<RateLimiterManager>(metrics);
    mgr->setAlgorithmFactory([]() {
        return std::make_unique<TokenBucket>(10.0, 0.0);
    });
    return mgr;
}

class PersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs::remove(kTestFile);
        mgr_ = makeManager();
        pm_  = std::make_unique<PersistenceManager>(mgr_, kTestFile.string());
    }
    void TearDown() override {
        fs::remove(kTestFile);
    }
    std::shared_ptr<RateLimiterManager> mgr_;
    std::unique_ptr<PersistenceManager> pm_;
};

TEST_F(PersistenceTest, LoadReturnsFalseWhenFileAbsent) {
    EXPECT_FALSE(pm_->load());
}

TEST_F(PersistenceTest, SaveWritesJsonFile) {
    mgr_->allowRequest("alice");
    pm_->save();
    EXPECT_TRUE(fs::exists(kTestFile));
    std::ifstream f(kTestFile);
    ASSERT_TRUE(f.is_open());
    nlohmann::json j;
    f >> j;
    EXPECT_TRUE(j.contains("alice"));
}

TEST_F(PersistenceTest, LoadRestoresUserState) {
    // Drain alice's bucket to 7 remaining (capacity=10, use 3).
    for (int i = 0; i < 3; i++) mgr_->allowRequest("alice");
    pm_->save();

    // New manager + persistence manager — load into it.
    auto mgr2 = makeManager();
    PersistenceManager pm2(mgr2, kTestFile.string());
    ASSERT_TRUE(pm2.load());

    // 7 requests should succeed (tokens left), 8th should fail.
    for (int i = 0; i < 7; i++) EXPECT_TRUE(mgr2->allowRequest("alice"));
    EXPECT_FALSE(mgr2->allowRequest("alice"));
}

TEST_F(PersistenceTest, StopPerformsFinalSave) {
    mgr_->allowRequest("bob");
    pm_->start();
    pm_->stop();
    EXPECT_TRUE(fs::exists(kTestFile));
    std::ifstream f(kTestFile);
    nlohmann::json j; f >> j;
    EXPECT_TRUE(j.contains("bob"));
}
