#pragma once
#include "RateLimiterManager.hpp"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace rate_limiter {

class PersistenceManager {
public:
    PersistenceManager(std::shared_ptr<RateLimiterManager> manager,
                       std::string filepath,
                       std::chrono::seconds interval = std::chrono::seconds{30});
    ~PersistenceManager();

    // Load initial state, then spawn background thread for periodic saves.
    void start();
    // Flush one final save, then join the background thread.
    void stop();

    // Returns false if the file does not exist or cannot be parsed.
    [[nodiscard]] bool load();
    // Overwrites filepath with the current manager state.
    void save();

private:
    void runLoop();

    std::shared_ptr<RateLimiterManager> manager_;
    std::string                          filepath_;
    std::chrono::seconds                 interval_;

    std::thread        thread_;
    std::atomic<bool>  stopped_{false};
};

} // namespace rate_limiter
