#pragma once

#include "console/common/Types.hpp"

#include <chrono>
#include <map>
#include <mutex>
#include <set>

namespace console::utils {

/**
 * @brief In-memory token blacklist for invalidated JWT tokens
 *
 * Stores revoked tokens until their expiration time.
 * Automatically removes expired tokens to prevent memory leaks.
 */
class TokenBlacklist {
  public:
    static TokenBlacklist& instance();

    /**
     * @brief Add a token to the blacklist
     * @param token The JWT token to blacklist
     * @param expiry_time The expiration time of the token
     */
    void add_token(const String& token, std::chrono::system_clock::time_point expiry_time);

    /**
     * @brief Check if a token is blacklisted
     * @param token The token to check
     * @return true if the token is blacklisted
     */
    bool is_blacklisted(const String& token);

    /**
     * @brief Remove expired tokens from the blacklist
     */
    void cleanup_expired();

    /**
     * @brief Get the number of blacklisted tokens
     */
    size_t size() const;

  private:
    TokenBlacklist() = default;

    mutable std::mutex mutex_;
    std::map<String, std::chrono::system_clock::time_point> blacklisted_tokens_;
};

} // namespace console::utils
