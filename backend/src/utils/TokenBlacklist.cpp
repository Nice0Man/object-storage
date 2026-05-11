//
#include "console/utils/TokenBlacklist.hpp"

#include "console/common/Logger.hpp"

namespace console::utils {

TokenBlacklist&
TokenBlacklist::instance() {
    static TokenBlacklist instance;
    return instance;
}

void
TokenBlacklist::add_token(const String& token, std::chrono::system_clock::time_point expiry_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    blacklisted_tokens_[token] = expiry_time;
    CONSOLE_LOG_DEBUG("Token added to blacklist (total: {})", blacklisted_tokens_.size());
}

bool
TokenBlacklist::is_blacklisted(const String& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = blacklisted_tokens_.find(token);
    if (it == blacklisted_tokens_.end()) {
        return false;
    }

    // Check if token is expired
    auto now = std::chrono::system_clock::now();
    if (now >= it->second) {
        // Token expired, remove it
        blacklisted_tokens_.erase(it);
        return false;
    }

    return true;
}

void
TokenBlacklist::cleanup_expired() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    size_t removed = 0;

    for (auto it = blacklisted_tokens_.begin(); it != blacklisted_tokens_.end();) {
        if (now >= it->second) {
            it = blacklisted_tokens_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }

    if (removed > 0) {
        CONSOLE_LOG_DEBUG("Cleaned up {} expired tokens from blacklist (remaining: {})",
                          removed,
                          blacklisted_tokens_.size());
    }
}

size_t
TokenBlacklist::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return blacklisted_tokens_.size();
}

} // namespace console::utils
