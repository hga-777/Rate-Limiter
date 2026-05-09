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
    server_.set_exception_handler([](const httplib::Request& req, httplib::Response& res,
                                     std::exception_ptr ep) {
        try { if (ep) std::rethrow_exception(ep); }
        catch (const std::exception& e) {
            std::cerr << "[exception] " << req.path << ": " << e.what() << "\n";
        }
        res.status = 500;
    });

    // GET /health — quick liveness check
    server_.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });

    // POST /allow  {"user_id": "alice"}
    server_.Post("/allow", [this](const httplib::Request& req, httplib::Response& res) {
        try {
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
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                nlohmann::json{{"error", std::string(e.what())}}.dump(),
                "application/json");
        }
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
        std::string userId   = body["user_id"].get<std::string>();
        std::string algoType = body["algorithm"].get<std::string>();
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
                nlohmann::json{{"error", "unknown algorithm: " + algoType}}.dump(),
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
