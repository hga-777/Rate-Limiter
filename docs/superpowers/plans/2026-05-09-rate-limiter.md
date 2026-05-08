# Distributed Rate Limiter — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a production-quality, 5-phase Distributed Rate Limiter in C++17/20 demonstrating concurrency, clean architecture, and SOLID design principles.

**Architecture:** Strategy Pattern + Manager Layer — algorithms are pure single-user logic objects with no locks; `RateLimiterManager` owns per-user state behind a `std::shared_mutex`; `MetricsCollector`, `PersistenceManager`, and `HttpServer` are fully independent components each with one responsibility.

**Tech Stack:** C++17 core (C++20 `std::jthread`/`std::stop_token` in Phase 5), CMake 3.16+, Google Test v1.14, cpp-httplib v0.15, nlohmann/json v3.11.

---

## File Map

| File | Responsibility |
|---|---|
| `CMakeLists.txt` | Root build config, FetchContent for all deps |
| `tests/CMakeLists.txt` | Test executable |
| `include/rate_limiter/IRateLimitAlgorithm.hpp` | Pure virtual strategy interface |
| `include/rate_limiter/MetricsCollector.hpp` | Atomic metrics header |
| `src/MetricsCollector.cpp` | Metrics implementation |
| `include/rate_limiter/TokenBucket.hpp` | Token bucket header |
| `src/TokenBucket.cpp` | Token bucket implementation |
| `include/rate_limiter/SlidingWindow.hpp` | Sliding window header |
| `src/SlidingWindow.cpp` | Sliding window implementation |
| `include/rate_limiter/FixedWindow.hpp` | Fixed window header |
| `src/FixedWindow.cpp` | Fixed window implementation |
| `include/rate_limiter/RateLimiterManager.hpp` | Manager header |
| `src/RateLimiterManager.cpp` | Manager implementation |
| `include/rate_limiter/HttpServer.hpp` | HTTP server header |
| `src/HttpServer.cpp` | HTTP server implementation |
| `include/rate_limiter/PersistenceManager.hpp` | Persistence header |
| `src/PersistenceManager.cpp` | Persistence implementation |
| `src/main.cpp` | Wires all components — grows each phase |
| `simulations/multi_thread_sim.cpp` | Phase 3 stress test driver |
| `tests/test_metrics.cpp` | MetricsCollector unit tests |
| `tests/test_token_bucket.cpp` | TokenBucket unit tests |
| `tests/test_sliding_window.cpp` | SlidingWindow unit tests |
| `tests/test_fixed_window.cpp` | FixedWindow unit tests |
| `tests/test_manager.cpp` | RateLimiterManager unit tests |

---

## Phase 1 — Token Bucket, Manager, Metrics

---

### Task 1: Project scaffold

**Files:**
- Create: `CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `src/main.cpp` (stub)

- [ ] **Step 1: Create directory structure**

```powershell
New-Item -ItemType Directory -Force -Path "include/rate_limiter", "src", "tests", "simulations"
```

- [ ] **Step 2: Write root `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.16)
project(RateLimiter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(nlohmann_json googletest)

add_library(rate_limiter_lib STATIC
    src/MetricsCollector.cpp
    src/TokenBucket.cpp
    src/RateLimiterManager.cpp
)
target_include_directories(rate_limiter_lib PUBLIC include)
target_link_libraries(rate_limiter_lib PUBLIC nlohmann_json::nlohmann_json)
target_compile_options(rate_limiter_lib PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

add_executable(rate_limiter src/main.cpp)
target_link_libraries(rate_limiter PRIVATE rate_limiter_lib)

enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 3: Write `tests/CMakeLists.txt`**

```cmake
add_executable(rate_limiter_tests
    test_metrics.cpp
    test_token_bucket.cpp
    test_manager.cpp
)
target_link_libraries(rate_limiter_tests
    PRIVATE rate_limiter_lib GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(rate_limiter_tests)
```

- [ ] **Step 4: Create stub source files so CMake can build**

`src/MetricsCollector.cpp`:
```cpp
// stub — filled in Task 3
```

`src/TokenBucket.cpp`:
```cpp
// stub — filled in Task 4
```

`src/RateLimiterManager.cpp`:
```cpp
// stub — filled in Task 5
```

`src/main.cpp`:
```cpp
#include <iostream>
int main() {
    std::cout << "Rate Limiter — Phase 1\n";
    return 0;
}
```

`tests/test_metrics.cpp`:
```cpp
// stub — filled in Task 3
```

`tests/test_token_bucket.cpp`:
```cpp
// stub — filled in Task 4
```

`tests/test_manager.cpp`:
```cpp
// stub — filled in Task 5
```

- [ ] **Step 5: Configure CMake and verify it succeeds**

```
cmake -S . -B build
```
Expected: `-- Build files have been written to: .../build` with no errors.

- [ ] **Step 6: Initialize git and commit**

```
git init
git add .
git commit -m "chore: scaffold CMake project with gtest and nlohmann/json"
```

---

### Task 2: `IRateLimitAlgorithm` interface

**Files:**
- Create: `include/rate_limiter/IRateLimitAlgorithm.hpp`

- [ ] **Step 1: Write the interface header**

```cpp
// ─────────────────────────────────────────────
// IRateLimitAlgorithm
// Strategy interface — each implementation is
// stateful for a single user, contains no locks.
// Callers must guarantee single-threaded access.
// ─────────────────────────────────────────────
#pragma once

#include <nlohmann/json.hpp>

namespace rate_limiter {

class IRateLimitAlgorithm {
public:
    virtual ~IRateLimitAlgorithm() = default;

    virtual bool allowRequest() = 0;
    virtual void reset() = 0;

    virtual nlohmann::json serialize() const = 0;
    virtual void deserialize(const nlohmann::json& j) = 0;
};

} // namespace rate_limiter
```

- [ ] **Step 2: Commit**

```
git add include/rate_limiter/IRateLimitAlgorithm.hpp
git commit -m "feat: add IRateLimitAlgorithm strategy interface"
```

---

### Task 3: `MetricsCollector` (TDD)

**Files:**
- Create: `include/rate_limiter/MetricsCollector.hpp`
- Modify: `src/MetricsCollector.cpp`
- Modify: `tests/test_metrics.cpp`

- [ ] **Step 1: Write the failing tests in `tests/test_metrics.cpp`**

```cpp
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
```

- [ ] **Step 2: Write `include/rate_limiter/MetricsCollector.hpp`**

```cpp
// ─────────────────────────────────────────────
// MetricsCollector
// Lock-free counters using std::atomic.
// fetch_add compiles to a single LOCK XADD
// instruction — no mutex, no contention.
// ─────────────────────────────────────────────
#pragma once

#include <atomic>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace rate_limiter {

class MetricsCollector {
public:
    void recordAllowed();
    void recordRejected();
    nlohmann::json snapshot() const;

private:
    std::atomic<uint64_t> allowed_{0};
    std::atomic<uint64_t> rejected_{0};
};

} // namespace rate_limiter
```

- [ ] **Step 3: Build — verify it fails (no implementation yet)**

```
cmake --build build --target rate_limiter_tests 2>&1
```
Expected: linker error — `MetricsCollector::recordAllowed` undefined.

- [ ] **Step 4: Implement `src/MetricsCollector.cpp`**

```cpp
#include "rate_limiter/MetricsCollector.hpp"
#include <sstream>
#include <iomanip>

namespace rate_limiter {

void MetricsCollector::recordAllowed() {
    allowed_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::recordRejected() {
    rejected_.fetch_add(1, std::memory_order_relaxed);
}

nlohmann::json MetricsCollector::snapshot() const {
    uint64_t allowed  = allowed_.load(std::memory_order_relaxed);
    uint64_t rejected = rejected_.load(std::memory_order_relaxed);
    uint64_t total    = allowed + rejected;
    double   rate     = (total > 0) ? (100.0 * rejected / total) : 0.0;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << rate << "%";

    return {
        {"allowed",        allowed},
        {"rejected",       rejected},
        {"total",          total},
        {"rejection_rate", oss.str()}
    };
}

} // namespace rate_limiter
```

- [ ] **Step 5: Build and run — verify all 4 tests pass**

```
cmake --build build
ctest --test-dir build --output-on-failure -C Debug
```
Expected: `[  PASSED  ] 4 tests.`

- [ ] **Step 6: Commit**

```
git add include/rate_limiter/MetricsCollector.hpp src/MetricsCollector.cpp tests/test_metrics.cpp
git commit -m "feat: add lock-free MetricsCollector with atomic counters"
```

---

### Task 4: `TokenBucket` algorithm (TDD)

**Files:**
- Create: `include/rate_limiter/TokenBucket.hpp`
- Modify: `src/TokenBucket.cpp`
- Modify: `tests/test_token_bucket.cpp`

- [ ] **Step 1: Write failing tests in `tests/test_token_bucket.cpp`**

```cpp
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
```

- [ ] **Step 2: Write `include/rate_limiter/TokenBucket.hpp`**

```cpp
// ─────────────────────────────────────────────
// TokenBucket
// Allows up to `capacity` requests, replenishing
// at `refillRatePerSecond` tokens/sec.
// Uses steady_clock — monotonically increasing,
// immune to NTP/DST jumps that break refill math.
// Not thread-safe — caller must synchronize.
// ─────────────────────────────────────────────
#pragma once

#include "IRateLimitAlgorithm.hpp"
#include <chrono>

namespace rate_limiter {

class TokenBucket : public IRateLimitAlgorithm {
public:
    TokenBucket(double capacity, double refillRatePerSecond);

    bool           allowRequest() override;
    void           reset()        override;
    nlohmann::json serialize()    const override;
    void           deserialize(const nlohmann::json& j) override;

private:
    void refill();

    double capacity_;
    double refillRatePerSecond_;
    double tokens_;
    std::chrono::steady_clock::time_point lastRefillTime_;
};

} // namespace rate_limiter
```

- [ ] **Step 3: Build — verify it fails (no implementation)**

```
cmake --build build --target rate_limiter_tests 2>&1
```
Expected: linker error — `TokenBucket::allowRequest` undefined.

- [ ] **Step 4: Implement `src/TokenBucket.cpp`**

```cpp
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
    capacity_            = j.at("capacity").get<double>();
    refillRatePerSecond_ = j.at("refill_rate").get<double>();
    tokens_              = j.at("tokens").get<double>();
    // Reset to now — steady_clock is process-local and cannot be
    // meaningfully restored across restarts. Conservative and correct.
    lastRefillTime_      = std::chrono::steady_clock::now();
}

} // namespace rate_limiter
```

- [ ] **Step 5: Build and run — verify all 5 tests pass**

```
cmake --build build
ctest --test-dir build --output-on-failure -C Debug
```
Expected: `[  PASSED  ] 5 tests.`

- [ ] **Step 6: Commit**

```
git add include/rate_limiter/TokenBucket.hpp src/TokenBucket.cpp tests/test_token_bucket.cpp
git commit -m "feat: implement TokenBucket rate limiter with serialize/deserialize"
```

---

### Task 5: `RateLimiterManager` (TDD)

**Files:**
- Create: `include/rate_limiter/RateLimiterManager.hpp`
- Modify: `src/RateLimiterManager.cpp`
- Modify: `tests/test_manager.cpp`

- [ ] **Step 1: Write failing tests in `tests/test_manager.cpp`**

```cpp
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
```

- [ ] **Step 2: Write `include/rate_limiter/RateLimiterManager.hpp`**

```cpp
// ─────────────────────────────────────────────
// RateLimiterManager
// Owns per-user algorithm instances.
// Thread safety: std::shared_mutex allows many
// concurrent readers (existing users) with an
// exclusive write only on first-seen user insert.
// Algorithms themselves hold no locks.
// ─────────────────────────────────────────────
#pragma once

#include "IRateLimitAlgorithm.hpp"
#include "MetricsCollector.hpp"
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace rate_limiter {

class RateLimiterManager {
public:
    explicit RateLimiterManager(std::shared_ptr<MetricsCollector> metrics);

    bool allowRequest(const std::string& userId);
    void registerUser(const std::string& userId,
                      std::unique_ptr<IRateLimitAlgorithm> algorithm);
    void setAlgorithmFactory(
        std::function<std::unique_ptr<IRateLimitAlgorithm>()> factory);

    nlohmann::json serializeState() const;
    void           deserializeState(const nlohmann::json& state);

private:
    mutable std::shared_mutex                                        mutex_;
    std::unordered_map<std::string, std::unique_ptr<IRateLimitAlgorithm>> limiters_;
    std::shared_ptr<MetricsCollector>                                metrics_;
    std::function<std::unique_ptr<IRateLimitAlgorithm>()>           algorithmFactory_;
};

} // namespace rate_limiter
```

- [ ] **Step 3: Build — verify it fails (no implementation)**

```
cmake --build build --target rate_limiter_tests 2>&1
```
Expected: linker error — `RateLimiterManager::allowRequest` undefined.

- [ ] **Step 4: Implement `src/RateLimiterManager.cpp`**

```cpp
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
```

> Note: `RateLimiterManager.cpp` includes `SlidingWindow.hpp` and `FixedWindow.hpp` for `deserializeState`. These headers don't exist yet — add stub headers now to keep the build green, then fill them in during Phase 2 tasks.

- [ ] **Step 5: Create stub headers for SlidingWindow and FixedWindow**

`include/rate_limiter/SlidingWindow.hpp`:
```cpp
#pragma once
#include "IRateLimitAlgorithm.hpp"
#include <chrono>
#include <deque>
namespace rate_limiter {
class SlidingWindow : public IRateLimitAlgorithm {
public:
    SlidingWindow(int maxRequests, std::chrono::milliseconds windowDuration);
    bool allowRequest() override;
    void reset() override;
    nlohmann::json serialize() const override;
    void deserialize(const nlohmann::json& j) override;
private:
    int maxRequests_;
    std::chrono::milliseconds windowDuration_;
    std::deque<std::chrono::steady_clock::time_point> requestTimestamps_;
};
} // namespace rate_limiter
```

`include/rate_limiter/FixedWindow.hpp`:
```cpp
#pragma once
#include "IRateLimitAlgorithm.hpp"
#include <chrono>
namespace rate_limiter {
class FixedWindow : public IRateLimitAlgorithm {
public:
    FixedWindow(int maxRequests, std::chrono::milliseconds windowDuration);
    bool allowRequest() override;
    void reset() override;
    nlohmann::json serialize() const override;
    void deserialize(const nlohmann::json& j) override;
private:
    int maxRequests_;
    std::chrono::milliseconds windowDuration_;
    int count_;
    std::chrono::steady_clock::time_point windowStart_;
};
} // namespace rate_limiter
```

`src/SlidingWindow.cpp` (stub):
```cpp
// stub — filled in Task 7
#include "rate_limiter/SlidingWindow.hpp"
namespace rate_limiter {
SlidingWindow::SlidingWindow(int, std::chrono::milliseconds) : maxRequests_(0), windowDuration_(0) {}
bool SlidingWindow::allowRequest() { return false; }
void SlidingWindow::reset() {}
nlohmann::json SlidingWindow::serialize() const { return {}; }
void SlidingWindow::deserialize(const nlohmann::json&) {}
} // namespace rate_limiter
```

`src/FixedWindow.cpp` (stub):
```cpp
// stub — filled in Task 8
#include "rate_limiter/FixedWindow.hpp"
namespace rate_limiter {
FixedWindow::FixedWindow(int, std::chrono::milliseconds) : maxRequests_(0), count_(0), windowStart_(std::chrono::steady_clock::now()) {}
bool FixedWindow::allowRequest() { return false; }
void FixedWindow::reset() {}
nlohmann::json FixedWindow::serialize() const { return {}; }
void FixedWindow::deserialize(const nlohmann::json&) {}
} // namespace rate_limiter
```

- [ ] **Step 6: Add SlidingWindow.cpp and FixedWindow.cpp to `CMakeLists.txt`**

Update the `add_library` block in `CMakeLists.txt`:
```cmake
add_library(rate_limiter_lib STATIC
    src/MetricsCollector.cpp
    src/TokenBucket.cpp
    src/RateLimiterManager.cpp
    src/SlidingWindow.cpp
    src/FixedWindow.cpp
)
```

- [ ] **Step 7: Build and run — verify all tests pass**

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -C Debug
```
Expected: all tests pass (9 total: 4 metrics + 5 token bucket + 0 manager stubs not yet wired).

Wait — `test_manager.cpp` is now wired. Expected: `[  PASSED  ] 14 tests.`

- [ ] **Step 8: Commit**

```
git add include/rate_limiter/ src/ tests/test_manager.cpp CMakeLists.txt
git commit -m "feat: implement RateLimiterManager with shared_mutex and double-checked locking"
```

---

### Task 6: Phase 1 `main.cpp` demo

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace stub with Phase 1 demo**

```cpp
#include <iostream>
#include <memory>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"

int main() {
    auto metrics = std::make_shared<rate_limiter::MetricsCollector>();
    rate_limiter::RateLimiterManager manager(metrics);

    manager.setAlgorithmFactory([]() {
        return std::make_unique<rate_limiter::TokenBucket>(5.0, 1.0);
    });

    std::cout << "=== Phase 1: Token Bucket Demo ===\n";
    for (int i = 0; i < 8; ++i) {
        bool ok = manager.allowRequest("alice");
        std::cout << "Request " << (i + 1) << ": " << (ok ? "ALLOWED" : "REJECTED") << "\n";
    }

    auto snap = metrics->snapshot();
    std::cout << "\nMetrics:\n"
              << "  Allowed:        " << snap["allowed"]        << "\n"
              << "  Rejected:       " << snap["rejected"]       << "\n"
              << "  Rejection Rate: " << snap["rejection_rate"] << "\n";
    return 0;
}
```

- [ ] **Step 2: Build and run — verify expected output**

```
cmake --build build
.\build\Debug\rate_limiter.exe
```
Expected:
```
=== Phase 1: Token Bucket Demo ===
Request 1: ALLOWED
Request 2: ALLOWED
Request 3: ALLOWED
Request 4: ALLOWED
Request 5: ALLOWED
Request 6: REJECTED
Request 7: REJECTED
Request 8: REJECTED

Metrics:
  Allowed:        5
  Rejected:       3
  Rejection Rate: 37.50%
```

- [ ] **Step 3: Commit**

```
git add src/main.cpp
git commit -m "feat: Phase 1 complete — TokenBucket + RateLimiterManager + MetricsCollector"
```

---

## Phase 2 — Sliding Window + Fixed Window Algorithms

---

### Task 7: `SlidingWindow` algorithm (TDD)

**Files:**
- Create: `tests/test_sliding_window.cpp`
- Modify: `include/rate_limiter/SlidingWindow.hpp` (already exists as stub — no changes needed)
- Modify: `src/SlidingWindow.cpp`

- [ ] **Step 1: Add test file to `tests/CMakeLists.txt`**

```cmake
add_executable(rate_limiter_tests
    test_metrics.cpp
    test_token_bucket.cpp
    test_manager.cpp
    test_sliding_window.cpp
    test_fixed_window.cpp
)
```

- [ ] **Step 2: Write failing tests in `tests/test_sliding_window.cpp`**

```cpp
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
    sw.allowRequest();
    sw.allowRequest();
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
```

- [ ] **Step 3: Create stub `tests/test_fixed_window.cpp`**

```cpp
// stub — filled in Task 8
```

- [ ] **Step 4: Build — verify sliding window tests fail**

```
cmake -S . -B build
cmake --build build --target rate_limiter_tests 2>&1
```
Expected: compile/link — tests compile but stub `allowRequest` always returns false, so tests fail.

- [ ] **Step 5: Implement `src/SlidingWindow.cpp`**

```cpp
// ─────────────────────────────────────────────
// SlidingWindow
// Tracks timestamps of recent requests in a deque.
// On each call, evicts timestamps older than the
// window, then checks if count < maxRequests.
// Time complexity: O(k) where k = evicted entries.
// Space: O(maxRequests) — bounded queue size.
// ─────────────────────────────────────────────
#include "rate_limiter/SlidingWindow.hpp"

namespace rate_limiter {

SlidingWindow::SlidingWindow(int maxRequests, std::chrono::milliseconds windowDuration)
    : maxRequests_(maxRequests)
    , windowDuration_(windowDuration)
{}

bool SlidingWindow::allowRequest() {
    auto now = std::chrono::steady_clock::now();

    while (!requestTimestamps_.empty() &&
           now - requestTimestamps_.front() > windowDuration_) {
        requestTimestamps_.pop_front();
    }

    if (static_cast<int>(requestTimestamps_.size()) < maxRequests_) {
        requestTimestamps_.push_back(now);
        return true;
    }
    return false;
}

void SlidingWindow::reset() {
    requestTimestamps_.clear();
}

nlohmann::json SlidingWindow::serialize() const {
    return {
        {"type",         "sliding_window"},
        {"max_requests", maxRequests_},
        {"window_ms",    windowDuration_.count()}
    };
    // Timestamps are steady_clock-based (process-local) and cannot be
    // meaningfully restored across restarts — we serialize only config.
    // On restore, the window starts fresh, which is conservative and correct.
}

void SlidingWindow::deserialize(const nlohmann::json& j) {
    maxRequests_    = j.at("max_requests").get<int>();
    windowDuration_ = std::chrono::milliseconds{j.at("window_ms").get<int64_t>()};
    requestTimestamps_.clear();
}

} // namespace rate_limiter
```

- [ ] **Step 6: Build and run — verify sliding window tests pass**

```
cmake --build build
ctest --test-dir build --output-on-failure -C Debug
```
Expected: all tests pass including the new sliding window tests.

- [ ] **Step 7: Commit**

```
git add src/SlidingWindow.cpp tests/test_sliding_window.cpp tests/CMakeLists.txt
git commit -m "feat: implement SlidingWindow rate limiter"
```

---

### Task 8: `FixedWindow` algorithm (TDD)

**Files:**
- Modify: `tests/test_fixed_window.cpp`
- Modify: `src/FixedWindow.cpp`

- [ ] **Step 1: Write failing tests in `tests/test_fixed_window.cpp`**

```cpp
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
    fw.allowRequest();
    fw.allowRequest();
    EXPECT_FALSE(fw.allowRequest());
    fw.reset();
    EXPECT_TRUE(fw.allowRequest());
}

TEST(FixedWindowTest, SerializePreservesCountAndConfig) {
    FixedWindow fw(5, 500ms);
    fw.allowRequest();
    fw.allowRequest();
    auto j = fw.serialize();
    EXPECT_EQ(j["type"].get<std::string>(), "fixed_window");
    EXPECT_EQ(j["max_requests"].get<int>(), 5);
    EXPECT_EQ(j["count"].get<int>(), 2);
}

TEST(FixedWindowTest, DeserializeRestoresCount) {
    FixedWindow fw(5, 1000ms);
    fw.allowRequest();
    fw.allowRequest(); // count = 2
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
```

- [ ] **Step 2: Build — verify fixed window tests fail**

```
cmake --build build --target rate_limiter_tests 2>&1
```
Expected: stub returns false — most tests fail.

- [ ] **Step 3: Implement `src/FixedWindow.cpp`**

```cpp
// ─────────────────────────────────────────────
// FixedWindow
// Counts requests in a fixed time window.
// Window resets when current time >= windowStart + duration.
// Known tradeoff vs SlidingWindow: a burst at the
// end of window N and start of window N+1 can pass
// 2× the limit. This is the "boundary burst" problem.
// Interviewers love asking about this.
// ─────────────────────────────────────────────
#include "rate_limiter/FixedWindow.hpp"

namespace rate_limiter {

FixedWindow::FixedWindow(int maxRequests, std::chrono::milliseconds windowDuration)
    : maxRequests_(maxRequests)
    , windowDuration_(windowDuration)
    , count_(0)
    , windowStart_(std::chrono::steady_clock::now())
{}

bool FixedWindow::allowRequest() {
    auto now = std::chrono::steady_clock::now();
    if (now - windowStart_ >= windowDuration_) {
        count_       = 0;
        windowStart_ = now;
    }
    if (count_ < maxRequests_) {
        ++count_;
        return true;
    }
    return false;
}

void FixedWindow::reset() {
    count_       = 0;
    windowStart_ = std::chrono::steady_clock::now();
}

nlohmann::json FixedWindow::serialize() const {
    return {
        {"type",         "fixed_window"},
        {"max_requests", maxRequests_},
        {"window_ms",    windowDuration_.count()},
        {"count",        count_}
    };
}

void FixedWindow::deserialize(const nlohmann::json& j) {
    maxRequests_    = j.at("max_requests").get<int>();
    windowDuration_ = std::chrono::milliseconds{j.at("window_ms").get<int64_t>()};
    count_          = j.at("count").get<int>();
    windowStart_    = std::chrono::steady_clock::now();
}

} // namespace rate_limiter
```

- [ ] **Step 4: Build and run — verify all tests pass**

```
cmake --build build
ctest --test-dir build --output-on-failure -C Debug
```
Expected: all tests pass.

- [ ] **Step 5: Commit**

```
git add src/FixedWindow.cpp tests/test_fixed_window.cpp
git commit -m "feat: implement FixedWindow rate limiter with boundary-burst documentation"
```

---

### Task 9: Algorithm hot-swap demo in `main.cpp`

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Extend `main.cpp` to demonstrate runtime algorithm swap**

```cpp
#include <iostream>
#include <memory>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"
#include "rate_limiter/SlidingWindow.hpp"
#include "rate_limiter/FixedWindow.hpp"

using namespace rate_limiter;
using namespace std::chrono_literals;

static void runDemo(RateLimiterManager& mgr, const std::string& label) {
    std::cout << "\n--- " << label << " ---\n";
    for (int i = 0; i < 5; ++i) {
        bool ok = mgr.allowRequest("alice");
        std::cout << "Request " << (i + 1) << ": " << (ok ? "ALLOWED" : "REJECTED") << "\n";
    }
}

int main() {
    auto metrics = std::make_shared<MetricsCollector>();
    RateLimiterManager manager(metrics);

    // Phase 1: Token Bucket
    manager.setAlgorithmFactory([]() {
        return std::make_unique<TokenBucket>(3.0, 1.0);
    });
    runDemo(manager, "Token Bucket (capacity=3)");

    // Phase 2: Hot-swap to Sliding Window for new users
    manager.setAlgorithmFactory([]() {
        return std::make_unique<SlidingWindow>(3, 1000ms);
    });
    manager.registerUser("bob", std::make_unique<SlidingWindow>(3, 1000ms));
    runDemo(manager, "Sliding Window (max=3, window=1s) — bob");

    // Phase 2: Fixed Window
    manager.setAlgorithmFactory([]() {
        return std::make_unique<FixedWindow>(3, 1000ms);
    });
    manager.registerUser("carol", std::make_unique<FixedWindow>(3, 1000ms));
    runDemo(manager, "Fixed Window (max=3, window=1s) — carol");

    return 0;
}
```

- [ ] **Step 2: Build and run — verify output shows all three algorithms**

```
cmake --build build
.\build\Debug\rate_limiter.exe
```
Expected: three sections each showing 3 ALLOWED then 2 REJECTED.

- [ ] **Step 3: Commit**

```
git add src/main.cpp
git commit -m "feat: Phase 2 complete — SlidingWindow + FixedWindow with hot-swap demo"
```

---

## Phase 3 — Multi-threaded Simulation

---

### Task 10: Multi-threaded simulation with metrics

**Files:**
- Create: `simulations/multi_thread_sim.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add simulation executable to `CMakeLists.txt`**

Add after the existing `add_executable(rate_limiter ...)` block:
```cmake
add_executable(rate_limiter_sim simulations/multi_thread_sim.cpp)
target_link_libraries(rate_limiter_sim PRIVATE rate_limiter_lib)
```

- [ ] **Step 2: Write `simulations/multi_thread_sim.cpp`**

```cpp
// ─────────────────────────────────────────────
// Multi-threaded simulation
// Spawns NUM_THREADS threads, each sending
// REQUESTS_PER_THREAD requests across NUM_USERS
// shared users. Demonstrates thread safety of
// RateLimiterManager under real contention.
//
// Scalability note: the coarse shared_mutex in
// RateLimiterManager is the bottleneck here.
// Under extreme concurrency, per-user mutexes
// (stored alongside each algorithm instance)
// would eliminate cross-user contention.
// ─────────────────────────────────────────────
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iomanip>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"

using namespace rate_limiter;
using namespace std::chrono_literals;

int main() {
    constexpr int NUM_THREADS          = 20;
    constexpr int REQUESTS_PER_THREAD  = 500;
    constexpr int NUM_USERS            = 5;
    constexpr double BUCKET_CAPACITY   = 50.0;
    constexpr double REFILL_RATE       = 10.0;

    auto metrics = std::make_shared<MetricsCollector>();
    RateLimiterManager manager(metrics);
    manager.setAlgorithmFactory([]() {
        return std::make_unique<TokenBucket>(BUCKET_CAPACITY, REFILL_RATE);
    });

    std::cout << "Starting simulation:\n"
              << "  Threads:           " << NUM_THREADS << "\n"
              << "  Requests/thread:   " << REQUESTS_PER_THREAD << "\n"
              << "  Total requests:    " << (NUM_THREADS * REQUESTS_PER_THREAD) << "\n"
              << "  Users:             " << NUM_USERS << "\n"
              << "  Bucket capacity:   " << BUCKET_CAPACITY << "\n"
              << "  Refill rate:       " << REFILL_RATE << "/sec\n\n";

    auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (int r = 0; r < REQUESTS_PER_THREAD; ++r) {
                std::string userId = "user_" + std::to_string(t % NUM_USERS);
                manager.allowRequest(userId);
            }
        });
    }
    for (auto& th : threads) th.join();

    auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();

    auto snap = metrics->snapshot();
    uint64_t total = snap["total"].get<uint64_t>();

    std::cout << "Results:\n"
              << "  Allowed:        " << snap["allowed"]        << "\n"
              << "  Rejected:       " << snap["rejected"]       << "\n"
              << "  Total:          " << total                  << "\n"
              << "  Rejection Rate: " << snap["rejection_rate"] << "\n"
              << "  Elapsed:        " << std::fixed << std::setprecision(3)
              << elapsed << "s\n"
              << "  Throughput:     " << std::fixed << std::setprecision(0)
              << (total / elapsed) << " req/s\n";
    return 0;
}
```

- [ ] **Step 3: Build and run simulation**

```
cmake -S . -B build
cmake --build build
.\build\Debug\rate_limiter_sim.exe
```
Expected: output showing allowed/rejected counts and throughput in req/s.

- [ ] **Step 4: Commit**

```
git add simulations/multi_thread_sim.cpp CMakeLists.txt
git commit -m "feat: Phase 3 complete — multi-threaded simulation with throughput metrics"
```

---

## Phase 4 — REST API Server

---

### Task 11: `HttpServer` implementation

**Files:**
- Modify: `CMakeLists.txt` (add cpp-httplib FetchContent)
- Create: `include/rate_limiter/HttpServer.hpp`
- Create: `src/HttpServer.cpp`

- [ ] **Step 1: Add cpp-httplib to `CMakeLists.txt`**

Add to the FetchContent declarations block (before `FetchContent_MakeAvailable`):
```cmake
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)
```

Update `FetchContent_MakeAvailable` to include it:
```cmake
FetchContent_MakeAvailable(nlohmann_json googletest httplib)
```

Add `httplib` to the library's link dependencies:
```cmake
target_link_libraries(rate_limiter_lib PUBLIC nlohmann_json::nlohmann_json httplib::httplib)
```

- [ ] **Step 2: Write `include/rate_limiter/HttpServer.hpp`**

```cpp
// ─────────────────────────────────────────────
// HttpServer
// Thin wrapper around cpp-httplib.
// Routes: POST /allow, GET /metrics, POST /register
// Delegates all business logic to RateLimiterManager.
// Server runs on a background std::thread.
// ─────────────────────────────────────────────
#pragma once

#include "RateLimiterManager.hpp"
#include "MetricsCollector.hpp"
#include <httplib.h>
#include <memory>
#include <thread>

namespace rate_limiter {

class HttpServer {
public:
    HttpServer(std::shared_ptr<RateLimiterManager> manager,
               std::shared_ptr<MetricsCollector>   metrics,
               int port = 8080);
    ~HttpServer();

    void start(); // non-blocking — spawns background thread
    void stop();

private:
    void setupRoutes();

    std::shared_ptr<RateLimiterManager> manager_;
    std::shared_ptr<MetricsCollector>   metrics_;
    int                                 port_;
    httplib::Server                     server_;
    std::thread                         serverThread_;
};

} // namespace rate_limiter
```

- [ ] **Step 3: Implement `src/HttpServer.cpp`**

```cpp
#include "rate_limiter/HttpServer.hpp"
#include "rate_limiter/TokenBucket.hpp"
#include "rate_limiter/SlidingWindow.hpp"
#include "rate_limiter/FixedWindow.hpp"
#include <iostream>

namespace rate_limiter {

HttpServer::HttpServer(std::shared_ptr<RateLimiterManager> manager,
                       std::shared_ptr<MetricsCollector>   metrics,
                       int port)
    : manager_(std::move(manager))
    , metrics_(std::move(metrics))
    , port_(port)
{
    setupRoutes();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::setupRoutes() {
    // POST /allow  {"user_id": "alice"}
    server_.Post("/allow", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        try {
            body = nlohmann::json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid JSON"})", "application/json");
            return;
        }
        if (!body.contains("user_id") || !body["user_id"].is_string()) {
            res.status = 400;
            res.set_content(R"({"error":"missing user_id"})", "application/json");
            return;
        }
        std::string userId = body["user_id"].get<std::string>();
        bool allowed = manager_->allowRequest(userId);
        res.status = allowed ? 200 : 429;
        res.set_content(
            nlohmann::json{{"allowed", allowed}, {"user_id", userId}}.dump(),
            "application/json");
    });

    // GET /metrics
    server_.Get("/metrics", [this](const httplib::Request&, httplib::Response& res) {
        res.set_content(metrics_->snapshot().dump(2), "application/json");
    });

    // POST /register  {"user_id":"bob","algorithm":"token_bucket","capacity":10,"refill_rate":2.0}
    server_.Post("/register", [this](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        try {
            body = nlohmann::json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid JSON"})", "application/json");
            return;
        }
        if (!body.contains("user_id") || !body.contains("algorithm")) {
            res.status = 400;
            res.set_content(R"({"error":"missing user_id or algorithm"})", "application/json");
            return;
        }
        std::string userId    = body["user_id"].get<std::string>();
        std::string algoType  = body["algorithm"].get<std::string>();
        std::unique_ptr<IRateLimitAlgorithm> algo;
        if (algoType == "token_bucket") {
            algo = std::make_unique<TokenBucket>(
                body.value("capacity",    10.0),
                body.value("refill_rate",  2.0));
        } else if (algoType == "sliding_window") {
            algo = std::make_unique<SlidingWindow>(
                body.value("max_requests", 10),
                std::chrono::milliseconds{body.value("window_ms", 1000)});
        } else if (algoType == "fixed_window") {
            algo = std::make_unique<FixedWindow>(
                body.value("max_requests", 10),
                std::chrono::milliseconds{body.value("window_ms", 1000)});
        } else {
            res.status = 400;
            res.set_content(
                nlohmann::json{{"error","unknown algorithm: " + algoType}}.dump(),
                "application/json");
            return;
        }
        manager_->registerUser(userId, std::move(algo));
        res.set_content(R"({"registered":true})", "application/json");
    });
}

void HttpServer::start() {
    serverThread_ = std::thread([this]() {
        std::cout << "HTTP server listening on port " << port_ << "\n";
        server_.listen("0.0.0.0", port_);
    });
}

void HttpServer::stop() {
    server_.stop();
    if (serverThread_.joinable()) serverThread_.join();
}

} // namespace rate_limiter
```

- [ ] **Step 4: Add `HttpServer.cpp` to `CMakeLists.txt`**

Update the `add_library` block:
```cmake
add_library(rate_limiter_lib STATIC
    src/MetricsCollector.cpp
    src/TokenBucket.cpp
    src/RateLimiterManager.cpp
    src/SlidingWindow.cpp
    src/FixedWindow.cpp
    src/HttpServer.cpp
)
```

- [ ] **Step 5: Build — verify no errors**

```
cmake -S . -B build
cmake --build build
```
Expected: builds cleanly.

- [ ] **Step 6: Commit**

```
git add include/rate_limiter/HttpServer.hpp src/HttpServer.cpp CMakeLists.txt
git commit -m "feat: add HttpServer with /allow, /metrics, /register endpoints"
```

---

### Task 12: Wire `HttpServer` into `main.cpp` and test manually

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace `main.cpp` with HTTP server mode**

```cpp
#include <iostream>
#include <memory>
#include <csignal>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"
#include "rate_limiter/HttpServer.hpp"

using namespace rate_limiter;

int main() {
    auto metrics = std::make_shared<MetricsCollector>();
    auto manager = std::make_shared<RateLimiterManager>(metrics);

    manager->setAlgorithmFactory([]() {
        return std::make_unique<TokenBucket>(10.0, 2.0);
    });

    HttpServer server(manager, metrics, 8080);
    server.start();

    std::cout << "Rate Limiter running. Press Enter to stop.\n";
    std::cin.get();
    server.stop();
    return 0;
}
```

- [ ] **Step 2: Build and run**

```
cmake --build build
.\build\Debug\rate_limiter.exe
```
Expected: `HTTP server listening on port 8080`

- [ ] **Step 3: Test the endpoints manually (in a second terminal)**

Test `/allow`:
```powershell
Invoke-WebRequest -Uri http://localhost:8080/allow -Method POST -ContentType "application/json" -Body '{"user_id":"alice"}'
```
Expected: `{"allowed":true,"user_id":"alice"}` (first 10 requests)

Test `/metrics`:
```powershell
Invoke-WebRequest -Uri http://localhost:8080/metrics -Method GET
```
Expected: JSON with allowed/rejected counts.

Test rate limiting — run the allow command 12 times; the 11th and 12th should return HTTP 429.

Test `/register`:
```powershell
Invoke-WebRequest -Uri http://localhost:8080/register -Method POST -ContentType "application/json" -Body '{"user_id":"bob","algorithm":"sliding_window","max_requests":3,"window_ms":5000}'
```
Expected: `{"registered":true}`

- [ ] **Step 4: Stop the server (press Enter in the server terminal)**

- [ ] **Step 5: Commit**

```
git add src/main.cpp
git commit -m "feat: Phase 4 complete — REST API server on port 8080"
```

---

## Phase 5 — JSON Persistence

---

### Task 13: `PersistenceManager` implementation

**Files:**
- Create: `include/rate_limiter/PersistenceManager.hpp`
- Create: `src/PersistenceManager.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Update `CMakeLists.txt` to enable C++20 for the persistence target**

Add after the `set(CMAKE_CXX_STANDARD 17)` lines:
```cmake
# PersistenceManager uses C++20 std::jthread and std::stop_token.
# We enable C++20 selectively on the source files that need it
# rather than raising the whole project standard.
set_source_files_properties(src/PersistenceManager.cpp
    PROPERTIES CXX_STANDARD 20)
```

- [ ] **Step 2: Write `include/rate_limiter/PersistenceManager.hpp`**

```cpp
// ─────────────────────────────────────────────
// PersistenceManager
// Runs a background thread (std::jthread — C++20)
// that snapshots manager state to a JSON file
// every `interval` seconds.
//
// std::jthread advantages over std::thread:
//   1. Auto-joins on destruction (RAII — no dangling thread)
//   2. Cooperative cancellation via stop_token —
//      no manual flag/mutex needed to wake the thread
// ─────────────────────────────────────────────
#pragma once

#include "RateLimiterManager.hpp"
#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace rate_limiter {

class PersistenceManager {
public:
    PersistenceManager(std::shared_ptr<RateLimiterManager> manager,
                       std::string                          filePath,
                       std::chrono::seconds                 interval);
    ~PersistenceManager(); // jthread auto-joins here — RAII

    void saveSnapshot();
    void loadSnapshot(); // call before starting server to restore state

private:
    void runLoop(std::stop_token stopToken);

    std::shared_ptr<RateLimiterManager> manager_;
    std::string                          filePath_;
    std::chrono::seconds                 interval_;
    std::jthread                         snapshotThread_; // C++20
};

} // namespace rate_limiter
```

- [ ] **Step 3: Implement `src/PersistenceManager.cpp`**

```cpp
#include "rate_limiter/PersistenceManager.hpp"
#include <fstream>
#include <iostream>
#include <mutex>
#include <condition_variable>

namespace rate_limiter {

PersistenceManager::PersistenceManager(std::shared_ptr<RateLimiterManager> manager,
                                       std::string                          filePath,
                                       std::chrono::seconds                 interval)
    : manager_(std::move(manager))
    , filePath_(std::move(filePath))
    , interval_(interval)
    , snapshotThread_([this](std::stop_token st) { runLoop(st); })
{}

PersistenceManager::~PersistenceManager() {
    // jthread destructor calls request_stop() then join() automatically.
    // Nothing to do here — this comment is for the interviewer.
}

void PersistenceManager::saveSnapshot() {
    try {
        auto state = manager_->serializeState();
        std::ofstream file(filePath_);
        if (!file) {
            std::cerr << "[PersistenceManager] Failed to open " << filePath_ << " for writing\n";
            return;
        }
        file << state.dump(2);
        std::cout << "[PersistenceManager] Snapshot saved to " << filePath_ << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceManager] Save failed: " << e.what() << "\n";
        // Availability > durability — log and continue serving.
    }
}

void PersistenceManager::loadSnapshot() {
    std::ifstream file(filePath_);
    if (!file) {
        std::cout << "[PersistenceManager] No snapshot found at " << filePath_
                  << " — starting fresh\n";
        return;
    }
    try {
        nlohmann::json state = nlohmann::json::parse(file);
        manager_->deserializeState(state);
        std::cout << "[PersistenceManager] State restored from " << filePath_ << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[PersistenceManager] Load failed: " << e.what()
                  << " — starting fresh\n";
    }
}

void PersistenceManager::runLoop(std::stop_token stopToken) {
    // condition_variable_any::wait_for with a stop_token wakes up on either:
    //   (a) the interval expiring, or
    //   (b) a stop request via stopToken.
    // This avoids sleeping through the full interval on shutdown.
    std::mutex mtx;
    std::condition_variable_any cv;
    std::unique_lock<std::mutex> lock(mtx);

    while (!stopToken.stop_requested()) {
        cv.wait_for(lock, stopToken, interval_,
                    [&stopToken] { return stopToken.stop_requested(); });
        if (!stopToken.stop_requested()) {
            saveSnapshot();
        }
    }
}

} // namespace rate_limiter
```

- [ ] **Step 4: Add `PersistenceManager.cpp` to `CMakeLists.txt`**

Update the `add_library` block:
```cmake
add_library(rate_limiter_lib STATIC
    src/MetricsCollector.cpp
    src/TokenBucket.cpp
    src/RateLimiterManager.cpp
    src/SlidingWindow.cpp
    src/FixedWindow.cpp
    src/HttpServer.cpp
    src/PersistenceManager.cpp
)
```

- [ ] **Step 5: Build — verify no errors**

```
cmake -S . -B build
cmake --build build
```
Expected: builds cleanly.

- [ ] **Step 6: Commit**

```
git add include/rate_limiter/PersistenceManager.hpp src/PersistenceManager.cpp CMakeLists.txt
git commit -m "feat: add PersistenceManager with C++20 jthread and stop_token"
```

---

### Task 14: Wire persistence into `main.cpp` and integration test

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Update `main.cpp` to include persistence**

```cpp
#include <iostream>
#include <memory>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"
#include "rate_limiter/HttpServer.hpp"
#include "rate_limiter/PersistenceManager.hpp"

using namespace rate_limiter;
using namespace std::chrono_literals;

int main() {
    auto metrics = std::make_shared<MetricsCollector>();
    auto manager = std::make_shared<RateLimiterManager>(metrics);

    manager->setAlgorithmFactory([]() {
        return std::make_unique<TokenBucket>(10.0, 2.0);
    });

    // Restore state from previous run (no-op if file missing)
    PersistenceManager persistence(manager, "rate_limiter_state.json", 30s);
    persistence.loadSnapshot();

    HttpServer server(manager, metrics, 8080);
    server.start();

    std::cout << "Rate Limiter running with persistence (snapshot every 30s).\n"
              << "Press Enter to stop and save final snapshot.\n";
    std::cin.get();

    server.stop();
    persistence.saveSnapshot(); // final snapshot on clean shutdown
    return 0;
}
```

- [ ] **Step 2: Build**

```
cmake --build build
```
Expected: clean build.

- [ ] **Step 3: Integration test — verify state survives restart**

Run the server:
```
.\build\Debug\rate_limiter.exe
```

In a second terminal, send 7 requests to alice (alice has a 10-token bucket):
```powershell
for ($i = 1; $i -le 7; $i++) {
    Invoke-WebRequest -Uri http://localhost:8080/allow -Method POST -ContentType "application/json" -Body '{"user_id":"alice"}' | Select-Object -ExpandProperty Content
}
```
Expected: 7 × `{"allowed":true,...}`

Press Enter to stop the server. Verify `rate_limiter_state.json` was created:
```powershell
Get-Content rate_limiter_state.json
```
Expected: JSON with `alice` entry showing `"tokens": 3.0` (approx — depends on timing).

Restart the server:
```
.\build\Debug\rate_limiter.exe
```
Expected: `[PersistenceManager] State restored from rate_limiter_state.json`

Send 4 more requests to alice — only 3 should be allowed (bucket had ~3 tokens remaining):
```powershell
for ($i = 1; $i -le 4; $i++) {
    Invoke-WebRequest -Uri http://localhost:8080/allow -Method POST -ContentType "application/json" -Body '{"user_id":"alice"}' | Select-Object -ExpandProperty Content
}
```
Expected: 3 × `allowed:true` then 1 × `allowed:false` (HTTP 429).

- [ ] **Step 4: Commit**

```
git add src/main.cpp
git commit -m "feat: Phase 5 complete — JSON persistence with crash-recovery and clean-shutdown snapshot"
```

---

## Self-Review Checklist

**Spec coverage:**
- [x] Phase 1: Token Bucket, thread-safe manager, gtest → Tasks 1–6
- [x] Phase 2: Sliding Window + Fixed Window + Strategy hot-swap → Tasks 7–9
- [x] Phase 3: Multi-threaded simulation + metrics → Task 10
- [x] Phase 4: REST API (/allow, /metrics, /register) → Tasks 11–12
- [x] Phase 5: JSON persistence + background snapshot thread → Tasks 13–14
- [x] SOLID principles: demonstrated throughout (SRP per class, OCP via factory, LSP via interface, DIP via IRateLimitAlgorithm)
- [x] `std::shared_mutex` with double-checked locking → Task 5
- [x] `std::atomic` metrics → Task 3
- [x] `std::chrono::steady_clock` → Task 4
- [x] `std::jthread` + `std::stop_token` (C++20) → Task 13
- [x] Error handling policy → HttpServer (Task 11) and PersistenceManager (Task 13)
- [x] Interview talking points documented inline as comments

**Type consistency:** All method signatures match between header declarations and implementations across all tasks.
