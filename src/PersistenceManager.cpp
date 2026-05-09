#include "rate_limiter/PersistenceManager.hpp"
#include <fstream>
#include <iostream>

namespace rate_limiter {

PersistenceManager::PersistenceManager(
    std::shared_ptr<RateLimiterManager> manager,
    std::string filepath,
    std::chrono::seconds interval)
    : manager_(std::move(manager))
    , filepath_(std::move(filepath))
    , interval_(interval)
{}

PersistenceManager::~PersistenceManager() {
    stop();
}

void PersistenceManager::start() {
    (void)load();  // best-effort restore; missing file is not an error at startup
    stopped_.store(false);
    thread_ = std::thread(&PersistenceManager::runLoop, this);
}

void PersistenceManager::stop() {
    // exchange returns the old value; if it was already true we already stopped.
    if (stopped_.exchange(true)) return;
    if (thread_.joinable()) thread_.join();
    save();
}

bool PersistenceManager::load() {
    std::ifstream file(filepath_);
    if (!file.is_open()) return false;
    try {
        nlohmann::json state = nlohmann::json::parse(file);
        manager_->deserializeState(state);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[persistence] load failed: " << e.what() << "\n";
        return false;
    }
}

void PersistenceManager::save() {
    try {
        nlohmann::json state = manager_->serializeState();
        std::ofstream file(filepath_);
        if (!file.is_open()) {
            std::cerr << "[persistence] cannot open " << filepath_ << " for writing\n";
            return;
        }
        file << state.dump(2);
    } catch (const std::exception& e) {
        std::cerr << "[persistence] save failed: " << e.what() << "\n";
    }
}

void PersistenceManager::runLoop() {
    using clock = std::chrono::steady_clock;
    auto next_save = clock::now() + interval_;
    // Poll in 50 ms ticks so stop() is noticed quickly without spinning.
    while (!stopped_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (stopped_.load()) break;
        if (clock::now() >= next_save) {
            save();
            next_save = clock::now() + interval_;
        }
    }
}

} // namespace rate_limiter
