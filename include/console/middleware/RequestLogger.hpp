#pragma once

#include "console/common/Types.hpp"

#include <drogon/HttpFilter.h>

#include <chrono>

namespace console::middleware {

/**
 * @brief Request logging middleware
 *
 * Logs HTTP requests and responses with timing information.
 * Useful for debugging, monitoring, and performance analysis.
 */
class RequestLogger : public drogon::HttpFilter<RequestLogger> {
  public:
    RequestLogger() = default;

    /**
     * @brief Process request and log details
     */
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;

  private:
    /**
     * @brief Format log message for request
     */
    String format_request_log(const drogon::HttpRequestPtr& req, std::chrono::milliseconds duration) const;

    /**
     * @brief Get client IP address
     */
    String get_client_ip(const drogon::HttpRequestPtr& req) const;

    /**
     * @brief Get request size in bytes
     */
    size_t get_request_size(const drogon::HttpRequestPtr& req) const;
};

} // namespace console::middleware
