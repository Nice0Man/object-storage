#include "console/api/PresignedController.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"
#include "console/common/ServiceLocator.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sstream>

namespace console::api {

namespace {
int
hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

String
decode_presigned_key_path(const String& encoded) {
    String out;
    out.reserve(encoded.size());
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            const int hi = hex_char_to_int(encoded[i + 1]);
            const int lo = hex_char_to_int(encoded[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(encoded[i]);
    }
    return out;
}
} // namespace

void
PresignedController::access_object(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   const String& bucket,
                                   const String& key) {
    // Get presigned parameters
    String expires_str = req->getParameter("expires");
    if (expires_str.empty()) {
        const String expires_in_str = req->getParameter("expires_in");
        if (!expires_in_str.empty()) {
            try {
                const int64_t expires_in = std::stoll(expires_in_str);
                const auto now = std::time(nullptr);
                expires_str = std::to_string(now + expires_in);
            } catch (...) {}
        }
    }
    String method = req->getParameter("method");
    String signature = req->getParameter("signature");
    const String canonical_key = decode_presigned_key_path(key);

    // If no presigned parameters, this might be a regular authenticated request
    // Return 401 to let the auth middleware handle it
    if (expires_str.empty() || signature.empty()) {
        Json::Value error;
        error["error"] = "Presigned URL parameters required (expires, signature)";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    // Parse expiration timestamp
    int64_t expires;
    try {
        expires = std::stoll(expires_str);
    } catch (...) {
        Json::Value error;
        error["error"] = "Invalid expiration timestamp";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // Check if URL has expired
    auto now = std::time(nullptr);
    if (now > expires) {
        CONSOLE_LOG_WARN("Presigned URL expired for {}/{}", bucket, key);
        Json::Value error;
        error["error"] = "Presigned URL has expired";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    // Default method to GET for access
    if (method.empty()) {
        method = "GET";
    }

    // Validate signature
    if (!validate_signature(bucket, canonical_key, expires, method, signature)) {
        CONSOLE_LOG_WARN("Invalid signature for presigned URL: {}/{}", bucket, canonical_key);
        Json::Value error;
        error["error"] = "Invalid signature";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    CONSOLE_LOG_DEBUG("Valid presigned URL access: {}/{}", bucket, canonical_key);

    // Get storage client and download object
    auto storage_client = ServiceLocator::storage_client();
    if (!storage_client) {
        Json::Value error;
        error["error"] = "Storage service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    auto result = storage_client->get_object(bucket, canonical_key);
    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    // Get object metadata for content type
    auto stat_result = storage_client->stat_object(bucket, canonical_key);
    String content_type = "application/octet-stream";
    if (stat_result) {
        content_type = stat_result.value().content_type();
    }

    // Record throughput stats
    int64_t data_size = static_cast<int64_t>(result.value().size());
    auto db = ServiceLocator::database();
    if (db) {
        storage::DataThroughputStats throughput;
        throughput.timestamp = std::time(nullptr);
        throughput.read_bytes = data_size;
        throughput.write_bytes = 0;
        throughput.total_bytes = data_size;
        db->add_throughput_stat(throughput);
    }

    // Return object data
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setBody(String(result.value().begin(), result.value().end()));
    resp->setContentTypeString(content_type);

    // Add content disposition header for downloads
    String filename = canonical_key;
    auto last_slash = canonical_key.rfind('/');
    if (last_slash != String::npos) {
        filename = canonical_key.substr(last_slash + 1);
    }
    resp->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    resp->addHeader("Content-Length", std::to_string(data_size));

    callback(resp);
}

void
PresignedController::upload_object(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   const String& bucket,
                                   const String& key) {
    // Get presigned parameters
    String expires_str = req->getParameter("expires");
    if (expires_str.empty()) {
        const String expires_in_str = req->getParameter("expires_in");
        if (!expires_in_str.empty()) {
            try {
                const int64_t expires_in = std::stoll(expires_in_str);
                const auto now = std::time(nullptr);
                expires_str = std::to_string(now + expires_in);
            } catch (...) {}
        }
    }
    String method = req->getParameter("method");
    String signature = req->getParameter("signature");
    const String canonical_key = decode_presigned_key_path(key);

    // If no presigned parameters, return 401
    if (expires_str.empty() || signature.empty()) {
        Json::Value error;
        error["error"] = "Presigned URL parameters required (expires, signature)";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    // Parse expiration timestamp
    int64_t expires;
    try {
        expires = std::stoll(expires_str);
    } catch (...) {
        Json::Value error;
        error["error"] = "Invalid expiration timestamp";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // Check if URL has expired
    auto now = std::time(nullptr);
    if (now > expires) {
        CONSOLE_LOG_WARN("Presigned URL expired for upload: {}/{}", bucket, key);
        Json::Value error;
        error["error"] = "Presigned URL has expired";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    // Default method to PUT for uploads
    if (method.empty()) {
        method = "PUT";
    }

    // Validate signature
    if (!validate_signature(bucket, canonical_key, expires, method, signature)) {
        CONSOLE_LOG_WARN("Invalid signature for presigned upload URL: {}/{}", bucket, canonical_key);
        Json::Value error;
        error["error"] = "Invalid signature";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    CONSOLE_LOG_DEBUG("Valid presigned URL upload: {}/{}", bucket, canonical_key);

    // Get storage client
    auto storage_client = ServiceLocator::storage_client();
    if (!storage_client) {
        Json::Value error;
        error["error"] = "Storage service not available";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Get body data
    auto body = req->getBody();
    ByteArray data(body.begin(), body.end());

    if (data.empty()) {
        Json::Value error;
        error["error"] = "Object data is required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // Get content type from header
    String content_type = req->getHeader("Content-Type");
    if (content_type.empty()) {
        content_type = "application/octet-stream";
    }

    // Upload object
    auto result = storage_client->put_object(bucket, canonical_key, data, content_type, {});
    if (!result) {
        Json::Value error;
        error["error"] = result.error();
        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Record throughput stats
    int64_t data_size = static_cast<int64_t>(data.size());
    auto db = ServiceLocator::database();
    if (db) {
        storage::DataThroughputStats throughput;
        throughput.timestamp = std::time(nullptr);
        throughput.read_bytes = 0;
        throughput.write_bytes = data_size;
        throughput.total_bytes = data_size;
        db->add_throughput_stat(throughput);
    }

    // Return success
    auto resp = drogon::HttpResponse::newHttpJsonResponse(result.value().to_json());
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}

bool
PresignedController::validate_signature(const String& bucket,
                                        const String& key,
                                        int64_t expires,
                                        const String& method,
                                        const String& signature) {
    // Generate expected signature
    String expected = generate_signature(bucket, key, expires, method);

    // Constant-time comparison to prevent timing attacks
    if (signature.length() != expected.length()) {
        return false;
    }

    int result = 0;
    for (size_t i = 0; i < signature.length(); ++i) {
        result |= (signature[i] ^ expected[i]);
    }

    return result == 0;
}

String
PresignedController::generate_signature(const String& bucket,
                                        const String& key,
                                        int64_t expires,
                                        const String& method) {
    // Build string to sign (same format as LocalStorageClient)
    std::ostringstream string_to_sign;
    string_to_sign << bucket << "/" << key << "\n" << expires << "\n" << method;

    // Get secret key from config - SECURITY: no fallback to hardcoded secrets
    auto& config = Config::instance();
    String secret_key = config.get<String>("presigned_url.secret_key").value_or(config.auth().jwt_secret);

    // SECURITY: Fail if no secret is configured
    if (secret_key.empty()) {
        CONSOLE_LOG_ERROR("Security: Presigned URL signature failed - no secret configured");
        return ""; // Return empty string to indicate failure
    }

    // Generate HMAC-SHA256
    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int hmac_len = 0;

    HMAC(EVP_sha256(),
         secret_key.c_str(),
         static_cast<int>(secret_key.length()),
         reinterpret_cast<const unsigned char*>(string_to_sign.str().c_str()),
         string_to_sign.str().length(),
         hmac_result,
         &hmac_len);

    // Convert to hex string
    std::ostringstream signature;
    signature << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < hmac_len; i++) {
        signature << std::setw(2) << static_cast<unsigned>(hmac_result[i]);
    }

    return signature.str();
}

} // namespace console::api
