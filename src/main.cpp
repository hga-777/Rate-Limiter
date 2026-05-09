#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <csignal>
#include "rate_limiter/RateLimiterManager.hpp"
#include "rate_limiter/MetricsCollector.hpp"
#include "rate_limiter/TokenBucket.hpp"
#include "rate_limiter/HttpServer.hpp"
#include "rate_limiter/PersistenceManager.hpp"

using namespace rate_limiter;

static std::atomic<bool> g_running{true};

extern "C" void signalHandler(int) {
    g_running = false;
}

int main() {
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    auto metrics = std::make_shared<MetricsCollector>();
    auto manager = std::make_shared<RateLimiterManager>(metrics);

    manager->setAlgorithmFactory([]() {
        return std::make_unique<TokenBucket>(10.0, 2.0);
    });

    // Restore state from previous run; periodic save every 30 s.
    PersistenceManager persistence(manager, "rate_limiter_state.json",
                                   std::chrono::seconds{30});
    persistence.start();

    HttpServer server(manager, metrics, 8080);
    server.start();

    std::cout << "Rate Limiter running on port 8080. Send SIGINT/SIGTERM to stop.\n";

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server.stop();
    persistence.stop();   // final save before exit
    return 0;
}
