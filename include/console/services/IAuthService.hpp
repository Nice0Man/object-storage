
#pragma once

#include "console/common/Types.hpp"
#include "console/models/Error.hpp"
#include "console/models/User.hpp"

#include <json/json.h>

namespace console::services {

/**
 * @brief Authentication and authorization service interface
 *
 * Handles user authentication through MinIO STS (Security Token Service),
 * token management, and session validation.
 */
class IAuthService {
  public:
    virtual ~IAuthService() = default;

    /**
     * @brief Authenticate user with username/password
     *
     * @param username User login
     * @param password User password
     * @return Result containing JWT token and user info, or error
     */
    virtual Result<Json::Value, models::ApiError> login(const String& username, const String& password) = 0;

    /**
     * @brief Logout user and invalidate token
     *
     * @param token JWT token to invalidate
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> logout(const String& token) = 0;

    /**
     * @brief Refresh JWT token
     *
     * @param refresh_token Current refresh token
     * @return Result containing new JWT token or error
     */
    virtual Result<Json::Value, models::ApiError> refresh_token(const String& refresh_token) = 0;

    /**
     * @brief Get current user information from token
     *
     * @param token JWT token
     * @return Result containing user info or error
     */
    virtual Result<models::User, models::ApiError> get_current_user(const String& token) = 0;

    /**
     * @brief Change user password
     *
     * @param token JWT token
     * @param old_password Current password
     * @param new_password New password
     * @return Result indicating success or error
     */
    virtual Result<void, models::ApiError> change_password(const String& token,
                                                           const String& old_password,
                                                           const String& new_password) = 0;

    /**
     * @brief Validate JWT token
     *
     * @param token JWT token
     * @return Result containing user info if valid, or error
     */
    virtual Result<UserInfo, models::ApiError> validate_token(const String& token) = 0;
};

} // namespace console::services
