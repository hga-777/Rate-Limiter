# Distributed Rate Limiter — Design Spec
**Date:** 2026-05-09
**Status:** Approved

---

## 1. Project Goal

Build a production-quality, interview-worthy Distributed Rate Limiter in Modern C++ that demonstrates:
- Low-level design and concurrency
- Clean architecture and SOLID principles
- Scalable backend thinking
- Modern C++17/20 practices

The project will be built incrementally (5 phases), committed to GitHub, and used as a portfolio piece for FAANG-style interviews.

---

## 2. Technology Choices

| Concern | Choice | Rationale |
|---|---|---|
| C++ Standard | C++17 core + C++20 sprinkles | Industry-safe baseline; deliberate C++20 features (jthread, stop_token) where they add clear value |
| Build System | CMake | Industry standard; FetchContent for dependencies |
| Testing | Google Test (gtest) | FAANG-standard; FetchContent integration |
| HTTP Library | cpp-httplib | Single-header, zero dependencies, transparent internals |
| Persistence Format | JSON (nlohmann/json) | Human-readable, easy to demo, single-header |

---

## 3. Architecture Overview

### Approach: Strategy Pattern + Manager Layer

Responsibilities are split into five distinct layers:

| Layer | Class | Responsibility |
|---|---|---|
| Algorithm | `IRateLimitAlgorithm` | Pure interface: `allowRequest()`, `serialize()`, `deserialize()` |
| Algorithm | `TokenBucket`, `SlidingWindow`, `FixedWindow` | Pure logic — no locks, no state maps |
| Orchestration | `RateLimiterManager` | Per-user map, `std::shared_mutex`, algorithm factory |
| Observability | `MetricsCollector` | `std::atomic` counters — lock-free, increment-only |
| Persistence | `PersistenceManager` | Background `std::jthread`, JSON snapshot every N seconds |
| Transport | `HttpServer` | Thin `cpp-httplib` wrapper — delegates to Manager |

**Key invariant:** Algorithms hold no locks and manage no maps. They are single-user, single-threaded objects. All thread safety lives at the manager boundary.

---

## 4. Folder Structure

```
rate-limiter/
├── CMakeLists.txt
├── include/
│   └── rate_limiter/
│       ├── IRateLimitAlgorithm.hpp
│       ├── TokenBucket.hpp
│       ├── SlidingWindow.hpp
│       ├── FixedWindow.hpp
│       ├── RateLimiterManager.hpp
│       ├── MetricsCollector.hpp
│       ├── PersistenceManager.hpp
│       └── HttpServer.hpp
├── src/
│   ├── TokenBucket.cpp
│   ├── SlidingWindow.cpp
│   ├── FixedWindow.cpp
│   ├── RateLimiterManager.cpp
│   ├── MetricsCollector.cpp
│   ├── PersistenceManager.cpp
│   ├── HttpServer.cpp
│   └── main.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── test_token_bucket.cpp
│   ├── test_sliding_window.cpp
│   ├── test_fixed_window.cpp
│   └── test_manager.cpp
├── simulations/
│   └── multi_thread_sim.cpp
└── docs/
    └── superpowers/
        └── specs/
            └── 2026-05-09-rate-limiter-design.md
```

---

## 5. Core Class Interfaces

### `IRateLimitAlgorithm`
```cpp
class IRateLimitAlgorithm {
public:
    virtual ~IRateLimitAlgorithm() = default;
    virtual bool allowRequest() = 0;
    virtual void reset() = 0;
    virtual nlohmann::json serialize() const = 0;
    virtual void deserialize(const nlohmann::json&) = 0;
};
```

### `TokenBucket`
```cpp
class TokenBucket : public IRateLimitAlgorithm {
public:
    TokenBucket(double capacity, double refillRatePerSecond);
    bool allowRequest() override;
    void reset() override;
    nlohmann::json serialize() const override;
    void deserialize(const nlohmann::json&) override;
private:
    double capacity_;
    double refillRatePerSecond_;
    double tokens_;
    std::chrono::steady_clock::time_point lastRefillTime_;
};
```
`steady_clock` is used over `system_clock` — system clock can jump backward (NTP, DST). Steady clock is monotonically increasing, essential for correct refill math.

### `RateLimiterManager`
```cpp
class RateLimiterManager {
public:
    explicit RateLimiterManager(std::shared_ptr<MetricsCollector> metrics);
    bool allowRequest(const std::string& userId);
    void registerUser(const std::string& userId,
                      std::unique_ptr<IRateLimitAlgorithm> algorithm);
    void setAlgorithmFactory(std::function<std::unique_ptr<IRateLimitAlgorithm>()> factory);
    nlohmann::json serializeState() const;
    void deserializeState(const nlohmann::json&);
private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<IRateLimitAlgorithm>> limiters_;
    std::shared_ptr<MetricsCollector> metrics_;
    std::function<std::unique_ptr<IRateLimitAlgorithm>()> algorithmFactory_;
};
```
`mutable` on the mutex allows `const` methods (e.g., `serializeState`) to acquire a read lock — a nuance most candidates miss.

### `MetricsCollector`
```cpp
class MetricsCollector {
public:
    void recordAllowed();
    void recordRejected();
    nlohmann::json snapshot() const;
private:
    std::atomic<uint64_t> allowed_{0};
    std::atomic<uint64_t> rejected_{0};
};
```

### `PersistenceManager`
```cpp
class PersistenceManager {
public:
    PersistenceManager(std::shared_ptr<RateLimiterManager> manager,
                       std::string filePath,
                       std::chrono::seconds interval);
    ~PersistenceManager();  // jthread auto-joins — RAII
    void saveSnapshot();
    void loadSnapshot();
private:
    void runLoop(std::stop_token stopToken);  // C++20
    std::shared_ptr<RateLimiterManager> manager_;
    std::string filePath_;
    std::chrono::seconds interval_;
    std::jthread snapshotThread_;             // C++20
};
```

---

## 6. Request Data Flow

```
Client HTTP Request
       │
       ▼
  HttpServer::handleAllow(userId)
       │
       ▼
  RateLimiterManager::allowRequest(userId)
       │
       ├─ shared_lock (read) ──► user exists?
       │                               │
       │                       NO ─────┤
       │                               ▼
       │                   unique_lock (write) → registerUser()
       │                               │
       │                       YES ◄───┘
       ▼
  algorithm->allowRequest()     ← pure logic, no locks
       │
       ├── true  → metrics.recordAllowed()  → HTTP 200
       └── false → metrics.recordRejected() → HTTP 429
```

Read-then-write on `shared_mutex` is a classic double-checked locking pattern. Used in caches, connection pools, and registries.

---

## 7. HTTP API Contract

### `POST /allow`
```
Request:      { "user_id": "alice" }
Response 200: { "allowed": true,  "user_id": "alice" }
Response 429: { "allowed": false, "user_id": "alice" }
```

### `GET /metrics`
```
Response 200: {
  "allowed":        14823,
  "rejected":       312,
  "total":          15135,
  "rejection_rate": "2.06%"
}
```

### `POST /register` *(Phase 4 optional extension)*
Allows runtime registration of a user with a specific algorithm and config. If omitted, the manager auto-registers unknown users on first `/allow` request using the configured factory default.
```
Request:      { "user_id": "bob", "algorithm": "token_bucket", "capacity": 10, "refill_rate": 2.0 }
Response 200: { "registered": true }
Response 400: { "error": "unknown algorithm" }
```

---

## 8. Error Handling Policy

| Scenario | Behavior |
|---|---|
| Malformed JSON body | HTTP 400, log and continue |
| Missing `user_id` field | HTTP 400 with descriptive message |
| Unknown algorithm in `/register` | HTTP 400, reject registration |
| Persistence file missing on startup | Log warning, start fresh — not fatal |
| Persistence write failure | Log error, continue serving — availability > durability |
| Algorithm throws internally | Catch at manager boundary, count as rejected, HTTP 500 |

**Availability > Durability:** Losing 30 seconds of state on a crash is acceptable. Dropping live traffic to protect state is not.

---

## 9. Thread Safety Model

| Concern | Mechanism | Rationale |
|---|---|---|
| Per-user map access | `std::shared_mutex` in manager | Multiple readers, occasional writers |
| Metrics counters | `std::atomic<uint64_t>` | Increment-only — no mutex needed |
| Algorithm state | No locks (manager holds exclusive lock first) | Single-threaded access per algorithm instance |
| Persistence snapshot | `std::jthread` + shared read lock | Reads state only, never writes |

**Known scalability bottleneck (Approach C):** The single `shared_mutex` in the manager becomes a contention point under very high concurrency. The evolution path is per-user mutexes stored alongside each algorithm instance — named explicitly as a Phase 3 interview discussion point.

---

## 10. Build Phases

| Phase | Feature | Key Concepts |
|---|---|---|
| 1 | Token Bucket, thread-safe manager, gtest | Strategy pattern, shared_mutex, steady_clock, RAII |
| 2 | Sliding Window + Fixed Window algorithms | Algorithm hot-swap, design patterns |
| 3 | Multi-threaded simulation, metrics | std::thread/std::async, atomics, contention analysis |
| 4 | REST API server (cpp-httplib) | HTTP, JSON, integration testing |
| 5 | JSON persistence, background snapshot thread | jthread, stop_token, crash recovery |

---

## 11. Interview Talking Points

- **Why `shared_mutex` over `mutex`?** Read-heavy workload — most requests are for existing users (reads). Exclusive lock only on new user registration (rare write). `shared_mutex` lets concurrent readers proceed without blocking each other.
- **Why atomics for metrics?** Metrics are increment-only monotonic counters. `std::atomic` increments compile to a single CPU instruction (`LOCK XADD`). No mutex overhead, no contention.
- **Why `steady_clock`?** `system_clock` is wall time — it can jump backward during NTP sync or DST changes. `steady_clock` is guaranteed monotonically increasing. Token refill math breaks if time goes backward.
- **Why `std::jthread` for persistence?** C++20 `jthread` auto-joins on destruction (RAII) and supports cooperative cancellation via `stop_token`. A raw `std::thread` requires manual `join()` in destructors — easy to forget, causing program termination.
- **Scalability to Redis/distributed systems:** This in-process rate limiter has a single point of truth. In a distributed system (multiple API gateway nodes), you'd move state to Redis using atomic Lua scripts or Redis' `INCR` + TTL for fixed windows. The algorithm interface (`serialize`/`deserialize`) is designed with this migration in mind.
