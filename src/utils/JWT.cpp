
//
#include "console/utils/JWT.hpp"

#include "console/common/Logger.hpp"

#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>

namespace console::utils {

// ============================================================================
// JWT Implementation
// ============================================================================

void
JWT::initialize(const String& secret, const String& encryption_passphrase, const String& encryption_salt) {
    if (secret.empty()) {
        throw std::invalid_argument("JWT secret cannot be empty");
    }
    if (encryption_passphrase.empty()) {
        throw std::invalid_argument("Encryption passphrase cannot be empty");
    }
    if (encryption_salt.empty()) {
        throw std::invalid_argument("Encryption salt cannot be empty");
    }

    secret_ = secret;
    encryption_passphrase_ = encryption_passphrase;
    encryption_salt_ = encryption_salt;
    initialized_ = true;

    CONSOLE_LOG_INFO("JWT manager initialized");
}

Result<String, String>
JWT::generate_token(const JWTClaims& claims) {
    if (!initialized_) {
        return Result<String, String>("JWT not initialized");
    }

    try {
        // Create JWT builder
        auto builder = create_token_builder(claims);

        // Sign with secret
        String token = builder.sign(jwt::algorithm::hs256{secret_});

        return Result<String, String>(token);
    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to generate JWT token: {}", e.what());
        return Result<String, String>(String("Failed to generate token: ") + e.what());
    }
}

Result<JWTClaims, String>
JWT::validate_token(const String& token) {
    if (!initialized_) {
        return Result<JWTClaims, String>("JWT not initialized");
    }

    try {
        // Decode token
        auto decoded = jwt::decode(token);

        // Create verifier
        auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{secret_}).with_issuer("console");

        // Verify signature and claims
        verifier.verify(decoded);

        // Extract claims
        JWTClaims claims = extract_claims(decoded);

        // Check expiration
        if (claims.is_expired()) {
            return Result<JWTClaims, String>("Token expired");
        }

        return Result<JWTClaims, String>(claims);
    } catch (const jwt::token_verification_exception& e) {
        CONSOLE_LOG_WARN("Token verification failed: {}", e.what());
        return Result<JWTClaims, String>(String("Token verification failed: ") + e.what());
    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to validate token: {}", e.what());
        return Result<JWTClaims, String>(String("Failed to validate token: ") + e.what());
    }
}

Result<String, String>
JWT::refresh_token(const String& token, Duration new_expiry) {
    auto claims_result = validate_token(token);
    if (claims_result.is_err()) {
        return Result<String, String>(claims_result.error());
    }

    auto claims = claims_result.value();

    // Update expiration
    claims.issued_at = std::chrono::system_clock::now();
    claims.expires_at = claims.issued_at + new_expiry;

    return generate_token(claims);
}

Result<JWTClaims, String>
JWT::decode_token_unsafe(const String& token) {
    try {
        auto decoded = jwt::decode(token);
        return Result<JWTClaims, String>(extract_claims(decoded));
    } catch (const std::exception& e) {
        return Result<JWTClaims, String>(String("Failed to decode token: ") + e.what());
    }
}

void
JWT::set_default_expiry(Duration expiry) {
    default_expiry_ = expiry;
}

Duration
JWT::get_default_expiry() {
    return default_expiry_;
}

jwt::builder
JWT::create_token_builder(const JWTClaims& claims) {
    auto builder = jwt::create();

    // Standard claims
    builder.set_issuer(claims.issuer);

    if (!claims.subject.empty()) {
        builder.set_subject(claims.subject);
    }

    if (!claims.audience.empty()) {
        builder.set_audience(claims.audience);
    }

    builder.set_issued_at(claims.issued_at);
    builder.set_expires_at(claims.expires_at);

    if (!claims.jti.empty()) {
        builder.set_id(claims.jti);
    }

    // Encrypt sensitive STS credentials
    String sensitive_data = claims.sts_access_key_id + "|" + claims.sts_secret_access_key + "|" +
                            claims.sts_session_token;

    auto encrypted_result = encrypt_claims(sensitive_data);
    if (encrypted_result.is_ok()) {
        builder.set_payload_claim("sts_encrypted", jwt::claim(encrypted_result.value()));
    }

    // Non-sensitive custom claims
    if (!claims.account_access_key.empty()) {
        builder.set_payload_claim("account_access_key", jwt::claim(claims.account_access_key));
    }

    if (!claims.account_name.empty()) {
        builder.set_payload_claim("account_name", jwt::claim(claims.account_name));
    }

    // Actions array
    if (!claims.actions.empty()) {
        picojson::array actions_array;
        for (const auto& action : claims.actions) {
            actions_array.push_back(picojson::value(action));
        }
        builder.set_payload_claim("actions", jwt::claim(picojson::value(actions_array)));
    }

    // Policies array
    if (!claims.policies.empty()) {
        picojson::array policies_array;
        for (const auto& policy : claims.policies) {
            policies_array.push_back(picojson::value(policy));
        }
        builder.set_payload_claim("policies", jwt::claim(picojson::value(policies_array)));
    }

    // Custom fields
    for (const auto& [key, value] : claims.custom_fields) {
        builder.set_payload_claim(key, jwt::claim(value));
    }

    return builder;
}

JWTClaims
JWT::extract_claims(const jwt::decoded_jwt<jwt::traits::kazuho_picojson>& decoded) {
    JWTClaims claims;

    // Standard claims
    if (decoded.has_issuer()) {
        claims.issuer = decoded.get_issuer();
    }

    if (decoded.has_subject()) {
        claims.subject = decoded.get_subject();
    }

    if (decoded.has_audience()) {
        auto audiences = decoded.get_audience();
        if (!audiences.empty()) {
            claims.audience = *audiences.begin();
        }
    }

    if (decoded.has_issued_at()) {
        claims.issued_at = decoded.get_issued_at();
    }

    if (decoded.has_expires_at()) {
        claims.expires_at = decoded.get_expires_at();
    }

    if (decoded.has_id()) {
        claims.jti = decoded.get_id();
    }

    // Decrypt STS credentials
    if (decoded.has_payload_claim("sts_encrypted")) {
        String encrypted = decoded.get_payload_claim("sts_encrypted").as_string();
        auto decrypted_result = decrypt_claims(encrypted);
        if (decrypted_result.is_ok()) {
            String decrypted = decrypted_result.value();
            // Parse "access_key|secret_key|session_token"
            size_t pos1 = decrypted.find('|');
            size_t pos2 = decrypted.find('|', pos1 + 1);
            if (pos1 != String::npos && pos2 != String::npos) {
                claims.sts_access_key_id = decrypted.substr(0, pos1);
                claims.sts_secret_access_key = decrypted.substr(pos1 + 1, pos2 - pos1 - 1);
                claims.sts_session_token = decrypted.substr(pos2 + 1);
            }
        }
    }

    // Custom claims
    if (decoded.has_payload_claim("account_access_key")) {
        claims.account_access_key = decoded.get_payload_claim("account_access_key").as_string();
    }

    if (decoded.has_payload_claim("account_name")) {
        claims.account_name = decoded.get_payload_claim("account_name").as_string();
    }

    // Actions array
    if (decoded.has_payload_claim("actions")) {
        auto actions_claim = decoded.get_payload_claim("actions");
        if (actions_claim.get_type() == jwt::json::type::array) {
            auto actions_array = actions_claim.as_array();
            for (const auto& action : actions_array) {
                if (action.is<std::string>()) {
                    claims.actions.push_back(action.get<std::string>());
                }
            }
        }
    }

    // Policies array
    if (decoded.has_payload_claim("policies")) {
        auto policies_claim = decoded.get_payload_claim("policies");
        if (policies_claim.get_type() == jwt::json::type::array) {
            auto policies_array = policies_claim.as_array();
            for (const auto& policy : policies_array) {
                if (policy.is<std::string>()) {
                    claims.policies.push_back(policy.get<std::string>());
                }
            }
        }
    }

    return claims;
}

Result<String, String>
JWT::encrypt_claims(const String& data) {
    // Derive encryption key using PBKDF2
    auto key = PBKDF2::derive_key(encryption_passphrase_, encryption_salt_);

    // Convert string to bytes
    ByteArray plaintext(data.begin(), data.end());

    // Encrypt using AES-GCM
    auto encrypted_result = AES_GCM::encrypt(plaintext, key);
    if (encrypted_result.is_err()) {
        return Result<String, String>(encrypted_result.error());
    }

    // Encode to base64
    String encoded = Base64::encode(encrypted_result.value());

    return Result<String, String>(encoded);
}

Result<String, String>
JWT::decrypt_claims(const String& encrypted_data) {
    // Decode from base64
    auto decoded_result = Base64::decode(encrypted_data);
    if (decoded_result.is_err()) {
        return Result<String, String>(decoded_result.error());
    }

    // Derive decryption key using PBKDF2
    auto key = PBKDF2::derive_key(encryption_passphrase_, encryption_salt_);

    // Decrypt using AES-GCM
    auto decrypted_result = AES_GCM::decrypt(decoded_result.value(), key);
    if (decrypted_result.is_err()) {
        return Result<String, String>(decrypted_result.error());
    }

    // Convert bytes to string
    String decrypted(decrypted_result.value().begin(), decrypted_result.value().end());

    return Result<String, String>(decrypted);
}

// ============================================================================
// PBKDF2 Implementation
// ============================================================================

ByteArray
PBKDF2::derive_key(const String& password, const String& salt, uint32_t iterations, uint32_t key_length) {
    ByteArray key(key_length);

    int result = PKCS5_PBKDF2_HMAC(password.c_str(),
                                   static_cast<int>(password.length()),
                                   reinterpret_cast<const unsigned char*>(salt.c_str()),
                                   static_cast<int>(salt.length()),
                                   static_cast<int>(iterations),
                                   EVP_sha256(),
                                   static_cast<int>(key_length),
                                   key.data());

    if (result != 1) {
        throw std::runtime_error("PBKDF2 key derivation failed");
    }

    return key;
}

// ============================================================================
// AES-GCM Implementation
// ============================================================================

Result<ByteArray, String>
AES_GCM::encrypt(const ByteArray& plaintext, const ByteArray& key) {
    if (key.size() != KEY_SIZE) {
        return Result<ByteArray, String>("Invalid key size");
    }

    // Generate random nonce
    ByteArray nonce(NONCE_SIZE);
    if (RAND_bytes(nonce.data(), NONCE_SIZE) != 1) {
        return Result<ByteArray, String>("Failed to generate nonce");
    }

    // Initialize cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Result<ByteArray, String>("Failed to create cipher context");
    }

    // Initialize encryption
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<ByteArray, String>("Failed to initialize encryption");
    }

    // Allocate output buffer
    ByteArray ciphertext(plaintext.size() + EVP_CIPHER_CTX_block_size(ctx));
    int len = 0;

    // Encrypt plaintext
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<ByteArray, String>("Encryption failed");
    }

    int ciphertext_len = len;

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<ByteArray, String>("Encryption finalization failed");
    }

    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    // Get authentication tag
    ByteArray tag(TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<ByteArray, String>("Failed to get authentication tag");
    }

    EVP_CIPHER_CTX_free(ctx);

    // Combine: nonce + ciphertext + tag
    ByteArray result;
    result.reserve(nonce.size() + ciphertext.size() + tag.size());
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    result.insert(result.end(), tag.begin(), tag.end());

    return Result<ByteArray, String>(result);
}

Result<ByteArray, String>
AES_GCM::decrypt(const ByteArray& ciphertext, const ByteArray& key) {
    if (key.size() != KEY_SIZE) {
        return Result<ByteArray, String>("Invalid key size");
    }

    if (ciphertext.size() < NONCE_SIZE + TAG_SIZE) {
        return Result<ByteArray, String>("Invalid ciphertext size");
    }

    // Extract components
    ByteArray nonce(ciphertext.begin(), ciphertext.begin() + NONCE_SIZE);
    ByteArray encrypted_data(ciphertext.begin() + NONCE_SIZE, ciphertext.end() - TAG_SIZE);
    ByteArray tag(ciphertext.end() - TAG_SIZE, ciphertext.end());

    // Initialize cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Result<ByteArray, String>("Failed to create cipher context");
    }

    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<ByteArray, String>("Failed to initialize decryption");
    }

    // Allocate output buffer
    ByteArray plaintext(encrypted_data.size() + EVP_CIPHER_CTX_block_size(ctx));
    int len = 0;

    // Decrypt ciphertext
    if (EVP_DecryptUpdate(
            ctx, plaintext.data(), &len, encrypted_data.data(), static_cast<int>(encrypted_data.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<ByteArray, String>("Decryption failed");
    }

    int plaintext_len = len;

    // Set authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, const_cast<unsigned char*>(tag.data())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<ByteArray, String>("Failed to set authentication tag");
    }

    // Finalize decryption (verifies tag)
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Result<ByteArray, String>("Decryption finalization failed (authentication failed)");
    }

    plaintext_len += len;
    plaintext.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);

    return Result<ByteArray, String>(plaintext);
}

// ============================================================================
// Base64 Implementation
// ============================================================================

static const String base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

String
Base64::encode(const ByteArray& data) {
    String encoded;
    int val = 0;
    int valb = -6;

    for (Byte c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (encoded.size() % 4) {
        encoded.push_back('=');
    }

    return encoded;
}

String
Base64::encode(const String& data) {
    return encode(ByteArray(data.begin(), data.end()));
}

Result<ByteArray, String>
Base64::decode(const String& encoded) {
    ByteArray decoded;
    Vector<int> T(256, -1);

    for (int i = 0; i < 64; i++) {
        T[static_cast<unsigned char>(base64_chars[i])] = i;
    }

    int val = 0;
    int valb = -8;

    for (char c : encoded) {
        if (T[static_cast<unsigned char>(c)] == -1) {
            break;
        }
        val = (val << 6) + T[static_cast<unsigned char>(c)];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(static_cast<Byte>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return Result<ByteArray, String>(decoded);
}

Result<String, String>
Base64::decode_string(const String& encoded) {
    auto result = decode(encoded);
    if (result.is_err()) {
        return Result<String, String>(result.error());
    }

    String decoded(result.value().begin(), result.value().end());
    return Result<String, String>(decoded);
}

} // namespace console::utils
