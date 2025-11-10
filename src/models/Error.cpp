// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0
//
#include "console/models/Error.hpp"

namespace console::models {

HttpStatus
ApiError::to_http_status() const {
    switch (code_) {
        // Authentication errors
        case ErrorCode::Unauthorized:
        case ErrorCode::InvalidCredentials:
        case ErrorCode::TokenExpired:
        case ErrorCode::TokenInvalid:
            return HttpStatus::Unauthorized;

        // Authorization errors
        case ErrorCode::Forbidden:
        case ErrorCode::InsufficientPermissions:
            return HttpStatus::Forbidden;

        // Not found errors
        case ErrorCode::NotFound:
        case ErrorCode::BucketNotFound:
        case ErrorCode::ObjectNotFound:
        case ErrorCode::UserNotFound:
        case ErrorCode::GroupNotFound:
        case ErrorCode::PolicyNotFound:
            return HttpStatus::NotFound;

        // Conflict errors
        case ErrorCode::AlreadyExists:
        case ErrorCode::BucketAlreadyExists:
        case ErrorCode::UserAlreadyExists:
        case ErrorCode::GroupAlreadyExists:
        case ErrorCode::PolicyAlreadyExists:
            return HttpStatus::Conflict;

        // Validation errors
        case ErrorCode::InvalidRequest:
        case ErrorCode::InvalidParameter:
        case ErrorCode::MissingParameter:
        case ErrorCode::ValidationFailed:
        case ErrorCode::InvalidBucketName:
        case ErrorCode::InvalidObjectKey:
        case ErrorCode::InvalidUserName:
        case ErrorCode::InvalidPolicy:
            return HttpStatus::BadRequest;

        // Internal errors
        case ErrorCode::InternalError:
        case ErrorCode::DatabaseError:
        case ErrorCode::ConfigurationError:
        case ErrorCode::StorageError:
        case ErrorCode::DiskFull:
        case ErrorCode::QuotaExceeded:
        default:
            return HttpStatus::InternalServerError;
    }
}

Json::Value
ApiError::to_json() const {
    Json::Value json;
    json["code"] = static_cast<int>(code_);
    json["message"] = message_;

    if (detail_) {
        json["detail"] = *detail_;
    }

    json["http_status"] = http_status_;

    return json;
}

ApiError
ApiError::from_json(const Json::Value& json) {
    ApiError error;
    error.code_ = static_cast<ErrorCode>(json.get("code", 0).asInt());
    error.message_ = json.get("message", "").asString();

    if (json.isMember("detail")) {
        error.detail_ = json["detail"].asString();
    }

    error.http_status_ = json.get("http_status", 500).asInt();

    return error;
}

ApiError
ApiError::unauthorized(const String& message) {
    ApiError error(ErrorCode::Unauthorized, message);
    error.set_http_status(static_cast<int>(HttpStatus::Unauthorized));
    return error;
}

ApiError
ApiError::forbidden(const String& message) {
    ApiError error(ErrorCode::Forbidden, message);
    error.set_http_status(static_cast<int>(HttpStatus::Forbidden));
    return error;
}

ApiError
ApiError::not_found(const String& resource) {
    ApiError error(ErrorCode::NotFound, resource + " not found");
    error.set_http_status(static_cast<int>(HttpStatus::NotFound));
    return error;
}

ApiError
ApiError::already_exists(const String& resource) {
    ApiError error(ErrorCode::AlreadyExists, resource + " already exists");
    error.set_http_status(static_cast<int>(HttpStatus::Conflict));
    return error;
}

ApiError
ApiError::invalid_request(const String& message) {
    ApiError error(ErrorCode::InvalidRequest, message);
    error.set_http_status(static_cast<int>(HttpStatus::BadRequest));
    return error;
}

ApiError
ApiError::internal_error(const String& message) {
    ApiError error(ErrorCode::InternalError, message);
    error.set_http_status(static_cast<int>(HttpStatus::InternalServerError));
    return error;
}

} // namespace console::models
