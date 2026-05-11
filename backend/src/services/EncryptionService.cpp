#include "console/services/EncryptionService.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"

#include <cstring>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdexcept>

namespace console::services {

namespace {
constexpr size_t AES_KEY_SIZE = 32; // AES-256
constexpr size_t AES_IV_SIZE = 16;  // AES block size
constexpr size_t AES_BLOCK_SIZE = 16;
} // namespace

EncryptionService::EncryptionService() {
    // Initialize OpenSSL (if needed)
    CONSOLE_LOG_DEBUG("EncryptionService initialized");
}

ByteArray
EncryptionService::generate_random_bytes(size_t size) {
    ByteArray bytes(size);
    if (RAND_bytes(bytes.data(), static_cast<int>(size)) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    return bytes;
}

ByteArray
EncryptionService::generate_key(size_t key_size) {
    return generate_random_bytes(key_size);
}

const ByteArray&
EncryptionService::get_master_key() {
    if (!master_key_initialized_) {
        // Try to get master key from config
        auto& config = Config::instance();
        String key_hex = config.get<String>("encryption.master_key").value_or("");

        if (key_hex.empty()) {
            // Generate a new master key
            master_key_ = generate_key(AES_KEY_SIZE);
            CONSOLE_LOG_WARN("Generated new encryption master key. Configure 'encryption.master_key' for persistence.");
        } else {
            // Parse hex string to bytes
            master_key_.resize(AES_KEY_SIZE);
            for (size_t i = 0; i < AES_KEY_SIZE && i * 2 < key_hex.size(); ++i) {
                master_key_[i] = static_cast<uint8_t>(std::stoul(key_hex.substr(i * 2, 2), nullptr, 16));
            }
        }
        master_key_initialized_ = true;
    }
    return master_key_;
}

Result<ByteArray, String>
EncryptionService::encrypt_data(const ByteArray& data, const ByteArray& key) {
    if (key.size() != AES_KEY_SIZE) {
        return Err<ByteArray, String>("Invalid key size. Expected 32 bytes for AES-256");
    }

    try {
        // Generate random IV
        ByteArray iv = generate_random_bytes(AES_IV_SIZE);

        // Create cipher context
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            return Err<ByteArray, String>("Failed to create cipher context");
        }

        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return Err<ByteArray, String>("Failed to initialize encryption");
        }

        // Allocate output buffer (input size + block size for padding)
        ByteArray ciphertext(data.size() + AES_BLOCK_SIZE);
        int len = 0;
        int ciphertext_len = 0;

        // Encrypt
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, data.data(), static_cast<int>(data.size())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return Err<ByteArray, String>("Failed to encrypt data");
        }
        ciphertext_len = len;

        // Finalize (handle padding)
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return Err<ByteArray, String>("Failed to finalize encryption");
        }
        ciphertext_len += len;

        EVP_CIPHER_CTX_free(ctx);

        // Resize ciphertext to actual size
        ciphertext.resize(ciphertext_len);

        // Prepend IV to ciphertext
        ByteArray result;
        result.reserve(AES_IV_SIZE + ciphertext_len);
        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());

        return Ok<ByteArray, String>(std::move(result));

    } catch (const std::exception& e) {
        return Err<ByteArray, String>(String("Encryption failed: ") + e.what());
    }
}

Result<ByteArray, String>
EncryptionService::decrypt_data(const ByteArray& encrypted_data, const ByteArray& key) {
    if (key.size() != AES_KEY_SIZE) {
        return Err<ByteArray, String>("Invalid key size. Expected 32 bytes for AES-256");
    }

    if (encrypted_data.size() < AES_IV_SIZE) {
        return Err<ByteArray, String>("Invalid encrypted data: too short");
    }

    try {
        // Extract IV from the beginning
        ByteArray iv(encrypted_data.begin(), encrypted_data.begin() + AES_IV_SIZE);
        ByteArray ciphertext(encrypted_data.begin() + AES_IV_SIZE, encrypted_data.end());

        // Create cipher context
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            return Err<ByteArray, String>("Failed to create cipher context");
        }

        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return Err<ByteArray, String>("Failed to initialize decryption");
        }

        // Allocate output buffer
        ByteArray plaintext(ciphertext.size());
        int len = 0;
        int plaintext_len = 0;

        // Decrypt
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size())) !=
            1) {
            EVP_CIPHER_CTX_free(ctx);
            return Err<ByteArray, String>("Failed to decrypt data");
        }
        plaintext_len = len;

        // Finalize (handle padding)
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return Err<ByteArray, String>("Failed to finalize decryption");
        }
        plaintext_len += len;

        EVP_CIPHER_CTX_free(ctx);

        // Resize plaintext to actual size
        plaintext.resize(plaintext_len);

        return Ok<ByteArray, String>(std::move(plaintext));

    } catch (const std::exception& e) {
        return Err<ByteArray, String>(String("Decryption failed: ") + e.what());
    }
}

Result<ByteArray, String>
EncryptionService::encrypt_for_storage(const ByteArray& data) {
    return encrypt_data(data, get_master_key());
}

Result<ByteArray, String>
EncryptionService::decrypt_from_storage(const ByteArray& encrypted_data) {
    return decrypt_data(encrypted_data, get_master_key());
}

Result<ByteArray, String>
EncryptionService::encrypt_with_customer_key(const ByteArray& data, const String& customer_key) {
    // Decode base64 key
    // For simplicity, expecting hex-encoded key here
    if (customer_key.size() != AES_KEY_SIZE * 2) {
        return Err<ByteArray, String>("Invalid customer key size. Expected 64 hex characters for AES-256");
    }

    ByteArray key(AES_KEY_SIZE);
    for (size_t i = 0; i < AES_KEY_SIZE; ++i) {
        key[i] = static_cast<uint8_t>(std::stoul(customer_key.substr(i * 2, 2), nullptr, 16));
    }

    return encrypt_data(data, key);
}

Result<ByteArray, String>
EncryptionService::decrypt_with_customer_key(const ByteArray& encrypted_data, const String& customer_key) {
    // Decode hex key
    if (customer_key.size() != AES_KEY_SIZE * 2) {
        return Err<ByteArray, String>("Invalid customer key size. Expected 64 hex characters for AES-256");
    }

    ByteArray key(AES_KEY_SIZE);
    for (size_t i = 0; i < AES_KEY_SIZE; ++i) {
        key[i] = static_cast<uint8_t>(std::stoul(customer_key.substr(i * 2, 2), nullptr, 16));
    }

    return decrypt_data(encrypted_data, key);
}

ByteArray
EncryptionService::derive_object_key(const String& bucket, const String& key) {
    // Derive object-specific key using SHA-256 of master key + bucket + key
    const auto& master = get_master_key();

    String data_to_hash;
    data_to_hash.append(reinterpret_cast<const char*>(master.data()), master.size());
    data_to_hash.append(bucket);
    data_to_hash.append(key);

    ByteArray derived_key(SHA256_DIGEST_LENGTH);
    SHA256(reinterpret_cast<const unsigned char*>(data_to_hash.data()), data_to_hash.size(), derived_key.data());

    return derived_key;
}

} // namespace console::services
