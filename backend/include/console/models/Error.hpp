#pragma once

#include "console/common/Types.hpp"

#include <exception>
#include <json/json.h>
#include <stdexcept>

namespace console::models {

/**
 * @brief Error codes
 */
enum class ErrorCode {
    // Generic errors
    Unknown = 0,
    InvalidRequest = 1000,
    InvalidParameter = 1001,
    MissingParameter = 1002,
    NotImplemented = 1003,

    // Authentication errors
    Unauthorized = 2000,
    InvalidCredentials = 2001,
    TokenExpired = 2002,
    TokenInvalid = 2003,

    // Authorization errors
    Forbidden = 3000,
    InsufficientPermissions = 3001,

    // Resource errors
    NotFound = 4000,
    BucketNotFound = 4001,
    ObjectNotFound = 4002,
    UserNotFound = 4003,
    GroupNotFound = 4004,
    PolicyNotFound = 4005,

    // Conflict errors
    AlreadyExists = 5000,
    BucketAlreadyExists = 5001,
    UserAlreadyExists = 5002,
    GroupAlreadyExists = 5003,
    PolicyAlreadyExists = 5004,

    // Validation errors
    ValidationFailed = 6000,
    InvalidBucketName = 6001,
    InvalidObjectKey = 6002,
    InvalidUserName = 6003,
    InvalidPolicy = 6004,

    // Storage errors
    StorageError = 7000,
    QuotaExceeded = 7001,
    DiskFull = 7002,

    // Network errors
    NetworkError = 8000,
    ConnectionTimeout = 8001,
    ConnectionRefused = 8002,

    // Internal errors
    InternalError = 9000,
    DatabaseError = 9001,
    ConfigurationError = 9002
};

/**
 * @brief Error response model
 */
class ApiError {
  public:
    ApiError() = default;
    ApiError(ErrorCode code, String message, Optional<String> detail = std::nullopt)
        : code_(code), message_(std::move(message)), detail_(std::move(detail)) {}

    // Constructor from HttpStatus
    ApiError(HttpStatus status, String message, Optional<String> detail = std::nullopt)
        : code_(http_status_to_error_code(status)), message_(std::move(message)), detail_(std::move(detail)),
          http_status_(static_cast<int>(status)) {}

    static ErrorCode http_status_to_error_code(HttpStatus status) {
        switch (status) {
            case HttpStatus::BadRequest:
                return ErrorCode::InvalidRequest;
            case HttpStatus::Unauthorized:
                return ErrorCode::Unauthorized;
            case HttpStatus::Forbidden:
                return ErrorCode::Forbidden;
            case HttpStatus::NotFound:
                return ErrorCode::NotFound;
            case HttpStatus::Conflict:
                return ErrorCode::AlreadyExists;
            case HttpStatus::NotImplemented:
                return ErrorCode::NotImplemented;
            case HttpStatus::InternalServerError:
                return ErrorCode::InternalError;
            default:
                return ErrorCode::InternalError;
        }
    }

    // Getters
    ErrorCode code() const { return code_; }
    const String& message() const { return message_; }
    const Optional<String>& detail() const { return detail_; }
    int http_status() const { return http_status_; }

    // Alias for compatibility
    HttpStatus status() const { return to_http_status(); }

    // Setters
    void set_code(ErrorCode code) { code_ = code; }
    void set_message(const String& message) { message_ = message; }
    void set_detail(const String& detail) { detail_ = detail; }
    void set_http_status(int status) { http_status_ = status; }

    // Utilities
    HttpStatus to_http_status() const;

    // Serialization
    Json::Value to_json() const;
    static ApiError from_json(const Json::Value& json);

    // Factory methods
    static ApiError unauthorized(const String& message = "Unauthorized");
    static ApiError forbidden(const String& message = "Forbidden");
    static ApiError not_found(const String& resource = "Resource");
    static ApiError already_exists(const String& resource = "Resource");
    static ApiError invalid_request(const String& message = "Invalid request");
    static ApiError internal_error(const String& message = "Internal server error");

  private:
    ErrorCode code_{ErrorCode::Unknown};
    String message_;
    Optional<String> detail_;
    int http_status_{500};
};

/**
 * @brief Exception class for API errors
 */
class ApiException : public std::runtime_error {
  public:
    explicit ApiException(const ApiError& error) : std::runtime_error(error.message()), error_(error) {}

    explicit ApiException(ErrorCode code, const String& message, const Optional<String>& detail = std::nullopt)
        : std::runtime_error(message), error_(code, message, detail) {}

    const ApiError& error() const { return error_; }

  private:
    ApiError error_;
};

/**
 * @brief Validation error details
 */
struct ValidationError {
    String field;
    String message;

    Json::Value to_json() const {
        Json::Value json;
        json["field"] = field;
        json["message"] = message;
        return json;
    }
};

/**
 * @brief Validation errors response
 */
class ValidationErrors {
  public:
    void add_error(const String& field, const String& message) { errors_.push_back({field, message}); }

    bool has_errors() const { return !errors_.empty(); }
    const Vector<ValidationError>& errors() const { return errors_; }

    Json::Value to_json() const {
        Json::Value json;
        json["code"] = static_cast<int>(ErrorCode::ValidationFailed);
        json["message"] = "Validation failed";

        Json::Value errors_array(Json::arrayValue);
        for (const auto& error : errors_) {
            errors_array.append(error.to_json());
        }
        json["errors"] = errors_array;

        return json;
    }

  private:
    Vector<ValidationError> errors_;
};

} // namespace console::models
