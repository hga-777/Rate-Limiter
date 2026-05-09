# Rate Limiter

A multi-algorithm HTTP rate limiter written in C++17. It exposes a REST API, persists state across restarts, and ships with a browser dashboard — no terminal commands needed to use it.

---

## Features

- **Three algorithms** — Token Bucket, Sliding Window, Fixed Window
- **Per-user limiters** — each user ID gets its own independent limiter
- **Hot-swap** — register a new algorithm for a user at runtime via the API
- **Persistence** — state is saved to JSON every 30 seconds and restored on restart
- **Live dashboard** — browser UI to register users, fire requests, and watch metrics
- **Thread-safe** — stress-tested with 20 concurrent threads × 500 requests each
- **28 unit tests** via Google Test

---

## Prerequisites

| Tool | Minimum version | Notes |
|------|----------------|-------|
| C++ compiler | MSVC 2019 / GCC 10 / Clang 12 | C++17 required |
| CMake | 3.16 | |
| Git | any | needed for CMake `FetchContent` |
| Internet connection | — | first build downloads dependencies |

> **Windows users:** install [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community is free) with the **Desktop development with C++** workload. CMake is bundled with it.
>
> **Linux/macOS users:** `sudo apt install cmake g++` or `brew install cmake`.

---

## Quick Start

### 1. Clone

```bash
git clone https://github.com/<your-username>/rate-limiter.git
cd rate-limiter
```

### 2. Configure

```bash
cmake -S . -B build
```

CMake will automatically download three dependencies on first run (takes ~1–2 min):

- [nlohmann/json](https://github.com/nlohmann/json) v3.11.3
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) v0.15.3
- [Google Test](https://github.com/google/googletest) v1.14.0

### 3. Build

```bash
# Release (recommended — fastest, no debug overhead)
cmake --build build --config Release

# Debug (for development)
cmake --build build --config Debug
```

Build output lands in `build/Release/` (Windows) or `build/` (Linux/macOS).

### 4. Run

**Windows — double-click:**

Navigate to `build/Release/` in File Explorer and double-click **`launch.bat`**.  
It starts the server in the background and opens `http://localhost:8080` in your browser automatically. Double-clicking again while the server is running just opens the browser without starting a second copy.

**Any platform — terminal:**

```bash
# Windows
.\build\Release\rate_limiter.exe

# Linux / macOS
./build/rate_limiter
```

Then open `http://localhost:8080` in your browser.

---

## Dashboard Walkthrough

Open `http://localhost:8080`. You will see four panels:

| Panel | What it does |
|-------|-------------|
| **Live Metrics** | Polls the server every 2 s — shows Allowed, Rejected, Total, Rejection Rate |
| **Register User** | Creates a per-user limiter with your chosen algorithm and parameters |
| **Check Rate Limit** | Fires a `POST /allow` for a user ID — green = allowed, red = rejected (429) |
| **Request Log** | Running log of every action in this browser session |

### Try it in 60 seconds

1. **Register** user `alice` → algorithm `Token Bucket` → capacity `3`, refill rate `0`
2. Click **Send Request** for `alice` three times
   - Requests 1 and 2: green **ALLOWED**
   - Request 3: red **REJECTED (429)**
3. Watch the Metrics panel update — Rejected count rises to 1

---

## REST API

All endpoints accept and return JSON. The dashboard calls these same endpoints.

### `POST /register`

Register or replace a rate limiter for a user.

```bash
curl -X POST http://localhost:8080/register \
  -H "Content-Type: application/json" \
  -d '{"user_id":"alice","algorithm":"token_bucket","capacity":10,"refill_rate":2.0}'
```

**Body fields:**

| Field | Required | Description |
|-------|----------|-------------|
| `user_id` | yes | Any string identifier |
| `algorithm` | yes | `token_bucket`, `sliding_window`, or `fixed_window` |
| `capacity` | token_bucket | Max token capacity |
| `refill_rate` | token_bucket | Tokens added per second |
| `max_requests` | sliding/fixed window | Max requests per window |
| `window_ms` | sliding/fixed window | Window size in milliseconds |

**Response:** `{"registered":true}`

---

### `POST /allow`

Check whether a request from a user should be allowed.

```bash
curl -X POST http://localhost:8080/allow \
  -H "Content-Type: application/json" \
  -d '{"user_id":"alice"}'
```

**Response (allowed):** `200 {"allowed":true,"user_id":"alice"}`  
**Response (rejected):** `429 {"allowed":false,"user_id":"alice"}`

> Users not yet registered get the server's default limiter (Token Bucket, capacity 10, refill 2/s).

---

### `GET /metrics`

Snapshot of server-wide counters since startup.

```bash
curl http://localhost:8080/metrics
```

```json
{
  "allowed": 42,
  "rejected": 7,
  "total": 49
}
```

---

### `GET /health`

Liveness check — returns `{"status":"ok"}` with HTTP 200.

---

## Algorithm Reference

### Token Bucket

```json
{"algorithm":"token_bucket","capacity":10,"refill_rate":2.0}
```

Tokens accumulate at `refill_rate` per second up to `capacity`. Each request costs 1 token. Good for allowing short bursts while enforcing a long-run average rate.

### Sliding Window

```json
{"algorithm":"sliding_window","max_requests":10,"window_ms":1000}
```

Counts requests in a rolling time window. Evenly distributes load — no boundary bursts. Good for strict per-second or per-minute caps.

### Fixed Window

```json
{"algorithm":"fixed_window","max_requests":10,"window_ms":1000}
```

Resets the counter at fixed clock boundaries (e.g., every second on the second). Simple and predictable, but up to 2× `max_requests` can slip through at a window boundary.

---

## Running Tests

```bash
cmake --build build --config Release --target rate_limiter_tests
cd build
ctest -C Release --output-on-failure
```

28 tests across 6 suites: TokenBucket, SlidingWindow, FixedWindow, RateLimiterManager, MetricsCollector, PersistenceManager.

---

## Running the Simulation

The multi-threaded simulation stress-tests thread safety: 20 concurrent threads, 500 requests each, across 5 shared users.

```bash
cmake --build build --config Release --target rate_limiter_sim
.\build\Release\rate_limiter_sim.exe   # Windows
./build/rate_limiter_sim               # Linux/macOS
```

---

## Project Structure

```
.
├── include/rate_limiter/   # Public headers (interfaces + class declarations)
│   ├── IRateLimitAlgorithm.hpp
│   ├── RateLimiterManager.hpp
│   ├── MetricsCollector.hpp
│   ├── PersistenceManager.hpp
│   ├── HttpServer.hpp
│   ├── TokenBucket.hpp
│   ├── SlidingWindow.hpp
│   └── FixedWindow.hpp
├── src/                    # Implementations
│   ├── main.cpp
│   └── *.cpp
├── tests/                  # Google Test suites (one file per component)
├── simulations/            # Multi-thread stress test
├── web/
│   └── index.html          # Single-file dashboard (no build step)
├── CMakeLists.txt
└── launch.bat              # Windows one-click launcher
```

---

## How Persistence Works

On startup the server reads `rate_limiter_state.json` (next to the executable) and restores all registered users and their limiter state. Every 30 seconds it overwrites the file with the current state. On clean shutdown (Ctrl+C or SIGTERM) a final save runs before exit.

The file is created automatically on first save — nothing to configure.

---

## Building on Linux / macOS

The project is cross-platform. Replace `.exe` paths with the plain binary name and use forward slashes:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/rate_limiter
```

`launch.bat` is Windows-only. On Linux/macOS start the binary directly and open `http://localhost:8080` in your browser.

---

## License

MIT — see [LICENSE](LICENSE) for details.
