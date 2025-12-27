#pragma once

#include "console/common/Types.hpp"

#include <chrono>
#include <jwt-cpp/jwt.h>

namespace console::utils {

/**
 * @brief JWT Claims structure
 */
struct JWTClaims {
    // Standard claims
    String issuer{"console"};
    String subject;
    String audience;
    TimePoint issued_at;
    TimePoint expires_at;
    String jti; // JWT ID

    // Custom claims - STS credentials (will be encrypted)
    String sts_access_key_id;
    String sts_secret_access_key;
    String sts_session_token;

    // Account info
    String account_access_key;
    String account_name;

    // Permissions
    Vector<String> actions;
    Vector<String> policies;

    // Additional metadata
    StringMap custom_fields;

    // Utilities
    bool is_expired() const { return std::chrono::system_clock::now() >= expires_at; }

    int64_t seconds_until_expiry() const {
        auto now = std::chrono::system_clock::now();
        if (expires_at <= now) {
            return 0;
        }
        return std::chrono::duration_cast<std::chrono::seconds>(expires_at - now).count();
    }
};

/**
 * @brief JWT Token Manager
 */
class JWT {
  public:
    /**
     * @brief Initialize JWT manager with secret
     * @param secret JWT signing secret
     * @param encryption_passphrase Passphrase for claims encryption
     * @param encryption_salt Salt for PBKDF2
     */
    static void initialize(const String& secret, const String& encryption_passphrase, const String& encryption_salt);

    /**
     * @brief Generate JWT token from claims
     * @param claims Claims to encode
     * @return JWT token string
     */
    static Result<String, String> generate_token(const JWTClaims& claims);

    /**
     * @brief Validate and parse JWT token
     * @param token JWT token string
     * @return Parsed claims or error
     */
    static Result<JWTClaims, String> validate_token(const String& token);

    /**
     * @brief Refresh token with new expiration
     * @param token Current token
     * @param new_expiry New expiration duration
     * @return New token or error
     */
    static Result<String, String> refresh_token(const String& token, Duration new_expiry = std::chrono::hours(24));

    /**
     * @brief Extract claims without validation (for debugging)
     * @param token JWT token string
     * @return Decoded claims (not validated!)
     */
    static Result<JWTClaims, String> decode_token_unsafe(const String& token);

    /**
     * @brief Set default token expiry duration
     */
    static void set_default_expiry(Duration expiry);

    /**
     * @brief Get default token expiry duration
     */
    static Duration get_default_expiry();

    /**
     * @brief Convert JWT claims to UserInfo
     * @param claims JWT claims
     * @return UserInfo structure
     */
    static UserInfo claims_to_userinfo(const JWTClaims& claims);

  private:
    /**
     * @brief Encrypt sensitive claims data
     * @param data Data to encrypt
     * @return Base64-encoded encrypted data
     */
    static Result<String, String> encrypt_claims(const String& data);

    /**
     * @brief Decrypt claims data
     * @param encrypted_data Base64-encoded encrypted data
     * @return Decrypted data
     */
    static Result<String, String> decrypt_claims(const String& encrypted_data);

    /**
     * @brief Convert claims to JWT payload
     */
    static jwt::builder<jwt::default_clock, jwt::traits::kazuho_picojson> create_token_builder(const JWTClaims& claims);

    /**
     * @brief Extract claims from decoded JWT
     */
    static JWTClaims extract_claims(const jwt::decoded_jwt<jwt::traits::kazuho_picojson>& decoded);

    // Configuration
    inline static String secret_;
    inline static String encryption_passphrase_;
    inline static String encryption_salt_;
    inline static Duration default_expiry_{std::chrono::hours(24)};
    inline static bool initialized_{false};
};

/**
 * @brief PBKDF2 key derivation
 */
class PBKDF2 {
  public:
    /**
     * @brief Derive key from password using PBKDF2
     * @param password Password/passphrase
     * @param salt Salt
     * @param iterations Number of iterations (default: 100000)
     * @param key_length Derived key length in bytes (default: 32)
     * @return Derived key
     */
    static ByteArray derive_key(const String& password,
                                const String& salt,
                                uint32_t iterations = 100000,
                                uint32_t key_length = 32);
};

/**
 * @brief AES-256-GCM encryption
 */
class AES_GCM {
  public:
    /**
     * @brief Encrypt data using AES-256-GCM
     * @param plaintext Data to encrypt
     * @param key Encryption key (must be 32 bytes for AES-256)
     * @return Encrypted data (nonce + ciphertext + tag)
     */
    static Result<ByteArray, String> encrypt(const ByteArray& plaintext, const ByteArray& key);

    /**
     * @brief Decrypt data using AES-256-GCM
     * @param ciphertext Encrypted data (nonce + ciphertext + tag)
     * @param key Encryption key (must be 32 bytes for AES-256)
     * @return Decrypted data
     */
    static Result<ByteArray, String> decrypt(const ByteArray& ciphertext, const ByteArray& key);

  private:
    static constexpr size_t KEY_SIZE = 32;   // AES-256
    static constexpr size_t NONCE_SIZE = 12; // GCM recommended
    static constexpr size_t TAG_SIZE = 16;   // GCM tag
};

/**
 * @brief Base64 encoding/decoding utilities
 */
class Base64 {
  public:
    static String encode(const ByteArray& data);
    static String encode(const String& data);
    static Result<ByteArray, String> decode(const String& encoded);
    static Result<String, String> decode_string(const String& encoded);
};

} // namespace console::utils
