
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
        return Err<String, String>("JWT not initialized");
    }

    try {
        // Create JWT builder
        auto builder = create_token_builder(claims);

        // Sign with secret
        String token = builder.sign(jwt::algorithm::hs256{secret_});

        return Ok<String, String>(std::move(token));
    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to generate JWT token: {}", e.what());
        return Err<String, String>(String("Failed to generate token: ") + e.what());
    }
}

Result<JWTClaims, String>
JWT::validate_token(const String& token) {
    if (!initialized_) {
        return Err<JWTClaims, String>("JWT not initialized");
    }

    try {
        // Decode token
        auto decoded = jwt::decode<JWT::json_traits>(token);

        auto verifier = jwt::verify<JWT::json_traits>()
                            .allow_algorithm(jwt::algorithm::hs256{secret_})
                            .with_issuer("object-storage-console");

        verifier.verify(decoded);

        // Extract claims
        JWTClaims claims = extract_claims(decoded);

        // Check expiration
        if (claims.is_expired()) {
            return Err<JWTClaims, String>("Token expired");
        }

        return Ok<JWTClaims, String>(std::move(claims));
    } catch (const std::exception& e) {
        CONSOLE_LOG_ERROR("Failed to validate token: {}", e.what());
        return Err<JWTClaims, String>(String("Failed to validate token: ") + e.what());
    }
}

Result<String, String>
JWT::refresh_token(const String& token, Duration new_expiry) {
    auto claims_result = validate_token(token);
    if (claims_result.is_err()) {
        return Err<String, String>(claims_result.error());
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
        auto decoded = jwt::decode<JWT::json_traits>(token);
        return Ok<JWTClaims, String>(extract_claims(decoded));
    } catch (const std::exception& e) {
        return Err<JWTClaims, String>(String("Failed to decode token: ") + e.what());
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

jwt::builder<jwt::default_clock, JWT::json_traits>
JWT::create_token_builder(const JWTClaims& claims) {
    auto builder = jwt::create<JWT::json_traits>();

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
    using claim_t = jwt::basic_claim<JWT::json_traits>;

    if (encrypted_result.is_ok()) {
        builder.set_payload_claim("sts_encrypted", claim_t(encrypted_result.value()));
    }

    if (!claims.account_access_key.empty()) {
        builder.set_payload_claim("account_access_key", claim_t(claims.account_access_key));
    }

    if (!claims.account_name.empty()) {
        builder.set_payload_claim("account_name", claim_t(claims.account_name));
    }

    if (!claims.actions.empty()) {
        nlohmann::json actions_array = nlohmann::json::array();
        for (const auto& action : claims.actions) {
            actions_array.push_back(action);
        }
        builder.set_payload_claim("actions", claim_t(actions_array));
    }

    if (!claims.policies.empty()) {
        nlohmann::json policies_array = nlohmann::json::array();
        for (const auto& policy : claims.policies) {
            policies_array.push_back(policy);
        }
        builder.set_payload_claim("policies", claim_t(policies_array));
    }

    for (const auto& [key, value] : claims.custom_fields) {
        builder.set_payload_claim(key, claim_t(value));
    }

    return builder;
}

JWTClaims
JWT::extract_claims(const jwt::decoded_jwt<JWT::json_traits>& decoded) {
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

    if (decoded.has_payload_claim("actions")) {
        auto actions_claim = decoded.get_payload_claim("actions");
        if (actions_claim.get_type() == jwt::json::type::array) {
            auto actions_array = actions_claim.as_array();
            for (const auto& action : actions_array) {
                if (action.is_string()) {
                    claims.actions.push_back(action.get<std::string>());
                }
            }
        }
    }

    if (decoded.has_payload_claim("policies")) {
        auto policies_claim = decoded.get_payload_claim("policies");
        if (policies_claim.get_type() == jwt::json::type::array) {
            auto policies_array = policies_claim.as_array();
            for (const auto& policy : policies_array) {
                if (policy.is_string()) {
                    claims.policies.push_back(policy.get<std::string>());
                }
            }
        }
    }

    // Extract known custom fields
    // Extract known custom fields explicitly
    if (decoded.has_payload_claim("is_admin")) {
        auto is_admin_claim = decoded.get_payload_claim("is_admin");
        CONSOLE_LOG_INFO("extract_claims: found is_admin claim, type={}", static_cast<int>(is_admin_claim.get_type()));
        if (is_admin_claim.get_type() == jwt::json::type::string) {
            claims.custom_fields["is_admin"] = is_admin_claim.as_string();
            CONSOLE_LOG_INFO("extract_claims: extracted is_admin as string: '{}'", is_admin_claim.as_string());
        } else {
            CONSOLE_LOG_WARN("extract_claims: is_admin claim is not a string, type={}",
                             static_cast<int>(is_admin_claim.get_type()));
        }
    } else {
        CONSOLE_LOG_WARN("extract_claims: no is_admin claim found in token");
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
        return Err<String, String>(encrypted_result.error());
    }

    // Encode to base64
    String encoded = Base64::encode(encrypted_result.value());

    return Ok<String, String>(std::move(encoded));
}

Result<String, String>
JWT::decrypt_claims(const String& encrypted_data) {
    // Decode from base64
    auto decoded_result = Base64::decode(encrypted_data);
    if (decoded_result.is_err()) {
        return Err<String, String>(decoded_result.error());
    }

    // Derive decryption key using PBKDF2
    auto key = PBKDF2::derive_key(encryption_passphrase_, encryption_salt_);

    // Decrypt using AES-GCM
    auto decrypted_result = AES_GCM::decrypt(decoded_result.value(), key);
    if (decrypted_result.is_err()) {
        return Err<String, String>(decrypted_result.error());
    }

    // Convert bytes to string
    String decrypted(decrypted_result.value().begin(), decrypted_result.value().end());

    return Ok<String, String>(std::move(decrypted));
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
        return Err<ByteArray, String>("Invalid key size");
    }

    // Generate random nonce
    ByteArray nonce(NONCE_SIZE);
    if (RAND_bytes(nonce.data(), NONCE_SIZE) != 1) {
        return Err<ByteArray, String>("Failed to generate nonce");
    }

    // Initialize cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Err<ByteArray, String>("Failed to create cipher context");
    }

    // Initialize encryption
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Err<ByteArray, String>("Failed to initialize encryption");
    }

    // Allocate output buffer
    ByteArray ciphertext(plaintext.size() + EVP_CIPHER_CTX_block_size(ctx));
    int len = 0;

    // Encrypt plaintext
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Err<ByteArray, String>("Encryption failed");
    }

    int ciphertext_len = len;

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Err<ByteArray, String>("Encryption finalization failed");
    }

    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    // Get authentication tag
    ByteArray tag(TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Err<ByteArray, String>("Failed to get authentication tag");
    }

    EVP_CIPHER_CTX_free(ctx);

    // Combine: nonce + ciphertext + tag
    ByteArray result;
    result.reserve(nonce.size() + ciphertext.size() + tag.size());
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    result.insert(result.end(), tag.begin(), tag.end());

    return Ok<ByteArray, String>(std::move(result));
}

Result<ByteArray, String>
AES_GCM::decrypt(const ByteArray& ciphertext, const ByteArray& key) {
    if (key.size() != KEY_SIZE) {
        return Err<ByteArray, String>("Invalid key size");
    }

    if (ciphertext.size() < NONCE_SIZE + TAG_SIZE) {
        return Err<ByteArray, String>("Invalid ciphertext size");
    }

    // Extract components
    ByteArray nonce(ciphertext.begin(), ciphertext.begin() + NONCE_SIZE);
    ByteArray encrypted_data(ciphertext.begin() + NONCE_SIZE, ciphertext.end() - TAG_SIZE);
    ByteArray tag(ciphertext.end() - TAG_SIZE, ciphertext.end());

    // Initialize cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Err<ByteArray, String>("Failed to create cipher context");
    }

    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Err<ByteArray, String>("Failed to initialize decryption");
    }

    // Allocate output buffer
    ByteArray plaintext(encrypted_data.size() + EVP_CIPHER_CTX_block_size(ctx));
    int len = 0;

    // Decrypt ciphertext
    if (EVP_DecryptUpdate(
            ctx, plaintext.data(), &len, encrypted_data.data(), static_cast<int>(encrypted_data.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Err<ByteArray, String>("Decryption failed");
    }

    int plaintext_len = len;

    // Set authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, const_cast<unsigned char*>(tag.data())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Err<ByteArray, String>("Failed to set authentication tag");
    }

    // Finalize decryption (verifies tag)
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Err<ByteArray, String>("Decryption finalization failed (authentication failed)");
    }

    plaintext_len += len;
    plaintext.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);

    return Ok<ByteArray, String>(std::move(plaintext));
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

    return Ok<ByteArray, String>(std::move(decoded));
}

Result<String, String>
Base64::decode_string(const String& encoded) {
    auto result = decode(encoded);
    if (result.is_err()) {
        return Err<String, String>(result.error());
    }

    String decoded(result.value().begin(), result.value().end());
    return Ok<String, String>(std::move(decoded));
}

// ============================================================================
// JWT - UserInfo Conversion Utility
// ============================================================================

UserInfo
JWT::claims_to_userinfo(const JWTClaims& claims) {
    UserInfo user_info;

    user_info.access_key = claims.account_access_key.empty() ? claims.subject : claims.account_access_key;
    user_info.secret_key = claims.sts_secret_access_key;
    user_info.session_token = claims.sts_session_token;
    user_info.account_name = claims.account_name;
    user_info.policies = claims.policies;
    user_info.created_at = claims.issued_at;

    // Check is_admin from custom fields
    auto it = claims.custom_fields.find("is_admin");
    user_info.is_admin = (it != claims.custom_fields.end() && it->second == "true");

    CONSOLE_LOG_INFO("claims_to_userinfo: access_key={}, is_admin={} (custom_fields has is_admin: {}, value: '{}')",
                     user_info.access_key,
                     user_info.is_admin,
                     (it != claims.custom_fields.end() ? "yes" : "no"),
                     (it != claims.custom_fields.end() ? it->second : "N/A"));

    return user_info;
}

} // namespace console::utils
