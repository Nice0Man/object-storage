#pragma once

#include "console/common/Types.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace console::utils {

/**
 * @brief Secure password hashing using PBKDF2-SHA256
 *
 * Provides industry-standard password hashing with:
 * - PBKDF2 key derivation
 * - SHA-256 hash function
 * - Random salt generation
 * - Configurable iteration count
 *
 * Hash format: $pbkdf2-sha256$iterations$salt$hash
 */
class PasswordHash {
  public:
    /**
     * @brief Hash a password securely
     *
     * @param password Plain text password to hash
     * @return Hashed password string in format: $pbkdf2-sha256$iterations$salt$hash
     *
     * @throws std::runtime_error if hashing fails
     */
    static String hash(const String& password);

    /**
     * @brief Verify a password against a hash
     *
     * @param password Plain text password to verify
     * @param hash Stored password hash
     * @return true if password matches, false otherwise
     */
    static bool verify(const String& password, const String& hash);

    /**
     * @brief Check if a string is a hashed password
     *
     * @param str String to check
     * @return true if string appears to be a hashed password
     */
    static bool is_hashed(const String& str);

    /**
     * Constant-time equality for legacy plaintext migration only (not for online passwords).
     */
    static bool secure_equals(const String& a, const String& b);

  private:
    static constexpr size_t SALT_LENGTH = 16;         // 128 bits
    static constexpr size_t HASH_LENGTH = 32;         // 256 bits
    static constexpr int DEFAULT_ITERATIONS = 310000; // OWASP recommendation 2023

    /**
     * @brief Generate cryptographically secure random salt
     */
    static ByteArray generate_salt();

    /**
     * @brief Perform PBKDF2-SHA256 key derivation
     */
    static ByteArray pbkdf2_sha256(const String& password, const ByteArray& salt, int iterations, size_t key_length);

    /**
     * @brief Encode binary data to base64
     */
    static String base64_encode(const ByteArray& data);

    /**
     * @brief Decode base64 string to binary data
     */
    static ByteArray base64_decode(const String& encoded);
};

} // namespace console::utils
