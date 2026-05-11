#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace console {

// ============================================================================
// Type Aliases
// ============================================================================

using Byte = uint8_t;
using ByteArray = std::vector<Byte>;
using String = std::string;
using StringView = std::string_view;
using StringMap = std::map<String, String>;

template <typename T>
using Optional = std::optional<T>;

template <typename T>
using UniquePtr = std::unique_ptr<T>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

template <typename T>
using Vector = std::vector<T>;

using TimePoint = std::chrono::system_clock::time_point;
using Duration = std::chrono::milliseconds;

// ============================================================================
// HTTP Types
// ============================================================================

enum class HttpMethod {
    Get,
    Post,
    Put,
    Delete,
    Patch,
    Options,
    Head
};

enum class HttpStatus {
    Ok = 200,
    Created = 201,
    NoContent = 204,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    Conflict = 409,
    InternalServerError = 500,
    NotImplemented = 501,
    ServiceUnavailable = 503
};

// ============================================================================
// Result Types
// ============================================================================

// Tag types for Result construction
struct OkTag {};
struct ErrTag {};
inline constexpr OkTag ok_tag{};
inline constexpr ErrTag err_tag{};

template <typename T, typename E = String>
class Result {
  public:
    // Tagged constructors to avoid ambiguity when T == E
    Result(OkTag, T value) : value_(std::move(value)), has_value_(true) {}
    Result(ErrTag, E error) : error_(std::move(error)), has_value_(false) {}

    bool is_ok() const noexcept { return has_value_; }
    bool is_err() const noexcept { return !has_value_; }

    // Conversion operators
    explicit operator bool() const noexcept { return has_value_; }
    bool operator!() const noexcept { return !has_value_; }

    T& value() & { return value_; }
    const T& value() const& { return value_; }
    T&& value() && { return std::move(value_); }

    E& error() & { return error_; }
    const E& error() const& { return error_; }
    E&& error() && { return std::move(error_); }

    T value_or(T default_value) const { return has_value_ ? value_ : std::move(default_value); }

    template <typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<T>())), E> {
        if (has_value_) {
            return Result<decltype(func(std::declval<T>())), E>(ok_tag, func(value_));
        }
        return Result<decltype(func(std::declval<T>())), E>(err_tag, error_);
    }

    template <typename F>
    auto and_then(F&& func) const {
        if (has_value_) {
            return func(value_);
        }
        using ReturnType = typename decltype(func(std::declval<T>()))::value_type;
        return Result<ReturnType, E>(err_tag, error_);
    }

  private:
    T value_;
    E error_;
    bool has_value_;
};

// Specialization for Result<void, E>
template <typename E>
class Result<void, E> {
  public:
    Result(OkTag) : has_value_(true) {}
    Result(ErrTag, E error) : error_(std::move(error)), has_value_(false) {}

    bool is_ok() const noexcept { return has_value_; }
    bool is_err() const noexcept { return !has_value_; }

    explicit operator bool() const noexcept { return has_value_; }
    bool operator!() const noexcept { return !has_value_; }

    E& error() & { return error_; }
    const E& error() const& { return error_; }
    E&& error() && { return std::move(error_); }

  private:
    E error_;
    bool has_value_;
};

// Helper functions for creating Result types
template <typename T, typename E = String>
inline Result<T, E>
Ok(T value) {
    return Result<T, E>(ok_tag, std::move(value));
}

template <typename E>
inline Result<void, E>
Ok() {
    return Result<void, E>(ok_tag);
}

template <typename T = bool, typename E = String>
inline Result<T, E>
Err(E error) {
    return Result<T, E>(err_tag, std::move(error));
}

// ============================================================================
// S3 Types
// ============================================================================

struct BucketInfo {
    String name;
    TimePoint creation_date;
    String region;
    int64_t size_bytes{0};         // Size of actual objects only
    int64_t size_with_metadata{0}; // Total size including metadata files
    int64_t object_count{0};
    bool versioning_enabled{false};
    bool encryption_enabled{false};
    String encryption_type; // "SSE-S3", "SSE-C", or empty
    StringMap tags;
};

struct ObjectInfo {
    String key;
    String bucket;
    int64_t size;
    TimePoint last_modified;
    String etag;
    String content_type;
    String storage_class;
    StringMap metadata;
    StringMap user_metadata;
    // Encryption fields
    bool encrypted{false};
    String encryption_algorithm; // "AES256" or empty
    String sse_type;             // "SSE-S3" or "SSE-C"
    String sse_customer_key_md5; // MD5 of customer key for SSE-C
    int64_t original_size{0};    // Original size before encryption
};

struct UserInfo {
    String access_key;
    String secret_key;
    String session_token;
    String account_name;
    String role{"viewer"};
    Vector<String> groups;
    Vector<String> policies;
    TimePoint created_at;
    bool is_admin{false};
};

// ============================================================================
// Configuration Types
// ============================================================================

struct ServerConfig {
    String host{"0.0.0.0"};
    uint16_t port{9090};
    uint16_t threads{static_cast<uint16_t>(std::thread::hardware_concurrency())};
    bool enable_ssl{false};
    String ssl_cert;
    String ssl_key;
    String log_level{"info"};
    String log_path{"logs/"};
};

struct S3Config {
    String endpoint;
    String access_key;
    String secret_key;
    String region{"us-east-1"};
    bool use_ssl{true};
    uint32_t timeout_ms{30000};
};

struct AuthConfig {
    String jwt_secret;
    Duration token_expiry{std::chrono::hours(24)};
    Duration refresh_token_expiry{std::chrono::hours(24 * 7)};
    bool enable_ldap{false};
    String ldap_server;
    uint16_t ldap_port{389};
};

struct DefaultAdminConfig {
    String username{"admin"};
    String password{"changeme"};
    String account_name{"Administrator"};
    bool enabled{true};
};

// ============================================================================
// Callback Types
// ============================================================================

using ErrorCallback = std::function<void(const String&)>;
using SuccessCallback = std::function<void()>;
using ProgressCallback = std::function<void(int64_t current, int64_t total)>;

// ============================================================================
// Date Utilities
// ============================================================================

/**
 * @brief Parse ISO 8601 date string to TimePoint
 *
 * Supports formats: "2024-12-03T10:30:00Z" and "2024-12-03"
 *
 * @param date_str ISO 8601 formatted date string
 * @return TimePoint parsed from string, or now() if parsing fails
 */
inline TimePoint
parse_iso8601_date(const String& date_str) {
    if (date_str.empty()) {
        return std::chrono::system_clock::now();
    }

    std::tm tm = {};

    // Try full ISO 8601 format with time
    if (date_str.length() >= 19) {
        // Parse "2024-12-03T10:30:00" format
        int year, month, day, hour, min, sec;
        if (sscanf(date_str.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
            tm.tm_year = year - 1900;
            tm.tm_mon = month - 1;
            tm.tm_mday = day;
            tm.tm_hour = hour;
            tm.tm_min = min;
            tm.tm_sec = sec;

#ifdef _WIN32
            std::time_t time = _mkgmtime(&tm);
#else
            std::time_t time = timegm(&tm);
#endif
            return std::chrono::system_clock::from_time_t(time);
        }
    }

    // Try date-only format
    if (date_str.length() >= 10) {
        int year, month, day;
        if (sscanf(date_str.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
            tm.tm_year = year - 1900;
            tm.tm_mon = month - 1;
            tm.tm_mday = day;

#ifdef _WIN32
            std::time_t time = _mkgmtime(&tm);
#else
            std::time_t time = timegm(&tm);
#endif
            return std::chrono::system_clock::from_time_t(time);
        }
    }

    return std::chrono::system_clock::now();
}

/**
 * @brief Format TimePoint to ISO 8601 string
 *
 * @param tp TimePoint to format
 * @return ISO 8601 formatted string "2024-12-03T10:30:00Z"
 */
inline String
format_iso8601_date(const TimePoint& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm* tm = std::gmtime(&time_t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", tm);
    return String(buffer);
}

} // namespace console
