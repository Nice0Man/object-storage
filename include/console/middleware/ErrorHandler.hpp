#pragma once

#include "console/common/Types.hpp"
#include "console/models/Error.hpp"

#include <drogon/HttpFilter.h>

namespace console::middleware {

/**
 * @brief Error handling middleware
 *
 * Catches exceptions and converts them to structured error responses.
 * Provides consistent error formatting across all API endpoints.
 */
class ErrorHandler : public drogon::HttpFilter<ErrorHandler> {
  public:
    ErrorHandler() = default;

    /**
     * @brief Process request with error handling
     */
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;

  private:
    /**
     * @brief Create error response from ApiError
     */
    drogon::HttpResponsePtr create_error_response(const models::ApiError& error) const;

    /**
     * @brief Create error response from exception
     */
    drogon::HttpResponsePtr create_error_response(const std::exception& ex) const;

    /**
     * @brief Create generic internal server error response
     */
    drogon::HttpResponsePtr create_internal_error_response() const;
};

} // namespace console::middleware
