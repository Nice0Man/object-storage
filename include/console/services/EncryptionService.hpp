#pragma once

#include "console/common/Types.hpp"

#include <json/json.h>
#include <memory>

namespace console::services {

/**
 * @brief Encryption algorithms supported
 */
enum class EncryptionAlgorithm {
    AES256, // AES-256-CBC
    AWSKMS  // AWS KMS (placeholder)
};

/**
 * @brief Server-Side Encryption configuration
 */
struct SSEConfig {
    EncryptionAlgorithm algorithm{EncryptionAlgorithm::AES256};
    String kms_key_id;       // For SSE-KMS
    String customer_key;     // For SSE-C (base64 encoded)
    String customer_key_md5; // For SSE-C verification

    bool is_enabled() const { return !customer_key.empty() || algorithm == EncryptionAlgorithm::AES256; }

    Json::Value to_json() const {
        Json::Value json;
        json["algorithm"] = algorithm == EncryptionAlgorithm::AES256 ? "AES256" : "aws:kms";
        if (!kms_key_id.empty()) {
            json["kms_key_id"] = kms_key_id;
        }
        return json;
    }
};

/**
 * @brief Bucket encryption configuration
 */
struct BucketEncryptionConfig {
    bool enabled{false};
    EncryptionAlgorithm default_algorithm{EncryptionAlgorithm::AES256};
    String kms_master_key_id;

    Json::Value to_json() const {
        Json::Value json;
        json["enabled"] = enabled;
        json["algorithm"] = default_algorithm == EncryptionAlgorithm::AES256 ? "AES256" : "aws:kms";
        if (!kms_master_key_id.empty()) {
            json["kms_master_key_id"] = kms_master_key_id;
        }
        return json;
    }

    static BucketEncryptionConfig from_json(const Json::Value& json) {
        BucketEncryptionConfig config;
        config.enabled = json.get("enabled", false).asBool();
        String algo = json.get("algorithm", "AES256").asString();
        config.default_algorithm = (algo == "aws:kms") ? EncryptionAlgorithm::AWSKMS : EncryptionAlgorithm::AES256;
        config.kms_master_key_id = json.get("kms_master_key_id", "").asString();
        return config;
    }
};

/**
 * @brief Server-Side Encryption Service
 *
 * Provides encryption/decryption functionality for object storage.
 * Supports:
 * - SSE-S3: Server-side encryption with S3-managed keys
 * - SSE-C: Server-side encryption with customer-provided keys
 */
class EncryptionService {
  public:
    EncryptionService();
    ~EncryptionService() = default;

    /**
     * @brief Encrypt data using AES-256
     *
     * @param data Data to encrypt
     * @param key Encryption key (32 bytes for AES-256)
     * @return Encrypted data with IV prepended
     */
    Result<ByteArray, String> encrypt_data(const ByteArray& data, const ByteArray& key);

    /**
     * @brief Decrypt data using AES-256
     *
     * @param encrypted_data Encrypted data (IV + ciphertext)
     * @param key Decryption key (32 bytes for AES-256)
     * @return Decrypted data
     */
    Result<ByteArray, String> decrypt_data(const ByteArray& encrypted_data, const ByteArray& key);

    /**
     * @brief Encrypt data for storage (SSE-S3)
     *
     * @param data Data to encrypt
     * @return Encrypted data with metadata
     */
    Result<ByteArray, String> encrypt_for_storage(const ByteArray& data);

    /**
     * @brief Decrypt data from storage (SSE-S3)
     *
     * @param encrypted_data Encrypted data from storage
     * @return Decrypted data
     */
    Result<ByteArray, String> decrypt_from_storage(const ByteArray& encrypted_data);

    /**
     * @brief Encrypt with customer-provided key (SSE-C)
     *
     * @param data Data to encrypt
     * @param customer_key Base64-encoded customer key
     * @return Encrypted data
     */
    Result<ByteArray, String> encrypt_with_customer_key(const ByteArray& data, const String& customer_key);

    /**
     * @brief Decrypt with customer-provided key (SSE-C)
     *
     * @param encrypted_data Encrypted data
     * @param customer_key Base64-encoded customer key
     * @return Decrypted data
     */
    Result<ByteArray, String> decrypt_with_customer_key(const ByteArray& encrypted_data, const String& customer_key);

    /**
     * @brief Generate a random encryption key
     *
     * @param key_size Key size in bytes (default 32 for AES-256)
     * @return Random key
     */
    ByteArray generate_key(size_t key_size = 32);

    /**
     * @brief Get or generate master key for SSE-S3
     */
    const ByteArray& get_master_key();

  private:
    ByteArray master_key_;
    bool master_key_initialized_{false};

    // Generate random bytes
    ByteArray generate_random_bytes(size_t size);

    // Derive key from master key and object info
    ByteArray derive_object_key(const String& bucket, const String& key);
};

} // namespace console::services
