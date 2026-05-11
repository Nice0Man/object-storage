#include "console/middleware/RateLimiter.hpp"

namespace console::middleware {

bool
RateLimiter::is_allowed(const std::string& key, const RateLimit& limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();

    if (++cleanup_counter_ > 100) {
        cleanup_old_entries(now);
        cleanup_counter_ = 0;
    }

    auto& entry = entries_[key];
    auto window_start = now - std::chrono::seconds(limit.window_seconds);

    while (!entry.empty() && entry.front() < window_start) {
        entry.pop();
    }

    if (static_cast<int>(entry.size()) >= limit.max_requests) {
        return false;
    }

    entry.push(now);
    return true;
}

void
RateLimiter::cleanup_old_entries(std::chrono::steady_clock::time_point now) {
    auto expiry = now - std::chrono::seconds(300);
    for (auto it = entries_.begin(); it != entries_.end();) {
        while (!it->second.empty() && it->second.front() < expiry) {
            it->second.pop();
        }
        if (it->second.empty()) {
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

RateLimiter&
global_rate_limiter() {
    static RateLimiter instance;
    return instance;
}

} // namespace console::middleware
