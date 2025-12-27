// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0

#pragma once

#include "console/clients/StorageClient.hpp"
#include "console/common/Config.hpp"
#include "console/services/IAuthService.hpp"
#include "console/storage/DatabaseManager.hpp"
#include "console/utils/JWT.hpp"

#include <memory>

namespace console::services {

/**
 * @brief Authentication service implementation
 *
 * Handles authentication through MinIO STS (Security Token Service),
 * JWT token generation/validation, and session management.
 */
class AuthService : public IAuthService {
  public:
    /**
     * @brief Construct AuthService with dependencies
     *
     * @param admin_client MinIO Admin client for admin operations
     * @param config Application configuration
     * @param db_manager Database manager for user authentication
     */
    AuthService(std::shared_ptr<clients::IMinioAdminClient> admin_client,
                std::shared_ptr<Config> config,
                std::shared_ptr<storage::DatabaseManager> db_manager);

    ~AuthService() override = default;

    // IAuthService implementation
    Result<Json::Value, models::ApiError> login(const String& username, const String& password) override;

    Result<void, models::ApiError> logout(const String& token) override;

    Result<Json::Value, models::ApiError> refresh_token(const String& refresh_token) override;

    Result<models::User, models::ApiError> get_current_user(const String& token) override;

    Result<void, models::ApiError> change_password(const String& token,
                                                   const String& old_password,
                                                   const String& new_password) override;

    Result<UserInfo, models::ApiError> validate_token(const String& token) override;

  private:
    /**
     * @brief Authenticate user with MinIO and get STS credentials
     *
     * @param username User login
     * @param password User password
     * @return Result containing UserInfo with STS credentials or error
     */
    Result<UserInfo, models::ApiError> authenticate_with_minio(const String& username, const String& password);

    /**
     * @brief Generate JWT access and refresh tokens
     *
     * @param user_info User information with STS credentials
     * @return JSON object with access_token, refresh_token, expires_in
     */
    Json::Value generate_tokens(const UserInfo& user_info);

    /**
     * @brief Validate credentials format
     *
     * @param username Username to validate
     * @param password Password to validate
     * @return Optional error if validation fails
     */
    Optional<models::ApiError> validate_credentials(const String& username, const String& password);

    /**
     * @brief Convert UserInfo to JWTClaims
     */
    utils::JWTClaims userinfo_to_claims(const UserInfo& user_info, Duration expiry);

    std::shared_ptr<clients::IMinioAdminClient> admin_client_;
    std::shared_ptr<Config> config_;
    std::shared_ptr<storage::DatabaseManager> db_manager_;

    // Token blacklist for logout (in production use Redis or similar)
    std::set<String> token_blacklist_;
    std::mutex blacklist_mutex_;
};

} // namespace console::services
