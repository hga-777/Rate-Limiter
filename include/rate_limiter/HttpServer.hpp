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
