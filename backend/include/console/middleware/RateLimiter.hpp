#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <queue>
#include <string>

namespace console::middleware {

class RateLimiter {
  public:
    struct RateLimit {
        int max_requests;
        int window_seconds;
    };

    bool is_allowed(const std::string& key, const RateLimit& limit);

  private:
    void cleanup_old_entries(std::chrono::steady_clock::time_point now);

    std::mutex mutex_;
    std::map<std::string, std::queue<std::chrono::steady_clock::time_point>> entries_;
    int cleanup_counter_ = 0;
};

RateLimiter& global_rate_limiter();

} // namespace console::middleware
