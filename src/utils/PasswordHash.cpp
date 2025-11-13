
#include "console/utils/PasswordHash.hpp"

#include "console/common/Logger.hpp"

#include <cstring>
#include <iomanip>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <stdexcept>

namespace console::utils {

String
PasswordHash::hash(const String& password) {
    if (password.empty()) {
        throw std::invalid_argument("Password cannot be empty");
    }

    try {
        // Generate random salt
        ByteArray salt = generate_salt();

        // Perform PBKDF2-SHA256
        ByteArray hash_bytes = pbkdf2_sha256(password, salt, DEFAULT_ITERATIONS, HASH_LENGTH);

        // Encode to base64
        String salt_b64 = base64_encode(salt);
        String hash_b64 = base64_encode(hash_bytes);

        // Format: $pbkdf2-sha256$iterations$salt$hash
        std::ostringstream oss;
        oss << "$pbkdf2-sha256$" << DEFAULT_ITERATIONS << "$" << salt_b64 << "$" << hash_b64;

        return oss.str();

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Password hashing failed: {}", e.what());
        throw std::runtime_error(String("Password hashing failed: ") + e.what());
    }
}

bool
PasswordHash::verify(const String& password, const String& hash) {
    if (password.empty() || hash.empty()) {
        return false;
    }

    try {
        // Check if this is a hashed password
        if (!is_hashed(hash)) {
            // For backward compatibility: plain text comparison
            CONSOLE_LOG_WARN("Comparing against unhashed password - this is insecure!");
            return password == hash;
        }

        // Parse hash format: $pbkdf2-sha256$iterations$salt$hash
        std::istringstream iss(hash);
        String part;
        Vector<String> parts;

        while (std::getline(iss, part, '$')) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }

        if (parts.size() != 4) {
            CONSOLE_LOG_ERROR("Invalid hash format");
            return false;
        }

        if (parts[0] != "pbkdf2-sha256") {
            CONSOLE_LOG_ERROR("Unsupported hash algorithm: {}", parts[0]);
            return false;
        }

        int iterations = std::stoi(parts[1]);
        String salt_b64 = parts[2];
        String stored_hash_b64 = parts[3];

        // Decode salt and hash
        ByteArray salt = base64_decode(salt_b64);
        ByteArray stored_hash = base64_decode(stored_hash_b64);

        // Hash the provided password with same salt and iterations
        ByteArray computed_hash = pbkdf2_sha256(password, salt, iterations, HASH_LENGTH);

        // Constant-time comparison to prevent timing attacks
        if (computed_hash.size() != stored_hash.size()) {
            return false;
        }

        int result = 0;
        for (size_t i = 0; i < computed_hash.size(); ++i) {
            result |= computed_hash[i] ^ stored_hash[i];
        }

        return result == 0;

    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Password verification failed: {}", e.what());
        return false;
    }
}

bool
PasswordHash::is_hashed(const String& str) {
    // Check if string starts with $pbkdf2-sha256$
    return str.find("$pbkdf2-sha256$") == 0;
}

// Private methods

ByteArray
PasswordHash::generate_salt() {
    ByteArray salt(SALT_LENGTH);

    if (RAND_bytes(salt.data(), static_cast<int>(SALT_LENGTH)) != 1) {
        throw std::runtime_error("Failed to generate random salt");
    }

    return salt;
}

ByteArray
PasswordHash::pbkdf2_sha256(const String& password, const ByteArray& salt, int iterations, size_t key_length) {
    ByteArray derived_key(key_length);

    int result = PKCS5_PBKDF2_HMAC(password.c_str(),
                                   static_cast<int>(password.length()),
                                   salt.data(),
                                   static_cast<int>(salt.size()),
                                   iterations,
                                   EVP_sha256(),
                                   static_cast<int>(key_length),
                                   derived_key.data());

    if (result != 1) {
        throw std::runtime_error("PBKDF2 key derivation failed");
    }

    return derived_key;
}

String
PasswordHash::base64_encode(const ByteArray& data) {
    BIO* bio_mem = BIO_new(BIO_s_mem());
    BIO* bio_b64 = BIO_new(BIO_f_base64());

    if (!bio_mem || !bio_b64) {
        if (bio_mem)
            BIO_free_all(bio_mem);
        if (bio_b64)
            BIO_free(bio_b64);
        throw std::runtime_error("Failed to create BIO objects");
    }

    bio_b64 = BIO_push(bio_b64, bio_mem);
    BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL); // No newlines

    BIO_write(bio_b64, data.data(), static_cast<int>(data.size()));
    BIO_flush(bio_b64);

    BUF_MEM* buf_ptr;
    BIO_get_mem_ptr(bio_b64, &buf_ptr);

    String result(buf_ptr->data, buf_ptr->length);

    BIO_free_all(bio_b64);

    return result;
}

ByteArray
PasswordHash::base64_decode(const String& encoded) {
    BIO* bio_mem = BIO_new_mem_buf(encoded.c_str(), static_cast<int>(encoded.length()));
    BIO* bio_b64 = BIO_new(BIO_f_base64());

    if (!bio_mem || !bio_b64) {
        if (bio_mem)
            BIO_free(bio_mem);
        if (bio_b64)
            BIO_free(bio_b64);
        throw std::runtime_error("Failed to create BIO objects");
    }

    bio_b64 = BIO_push(bio_b64, bio_mem);
    BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL); // No newlines

    ByteArray decoded(encoded.length()); // Max possible size
    int decoded_length = BIO_read(bio_b64, decoded.data(), static_cast<int>(decoded.size()));

    BIO_free_all(bio_b64);

    if (decoded_length < 0) {
        throw std::runtime_error("Base64 decoding failed");
    }

    decoded.resize(static_cast<size_t>(decoded_length));

    return decoded;
}

} // namespace console::utils
