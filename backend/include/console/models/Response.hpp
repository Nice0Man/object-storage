#pragma once

#include "console/common/Types.hpp"
#include "console/models/Error.hpp"

#include <json/json.h>

namespace console::models {

/**
 * @brief Generic API response wrapper
 */
template <typename T>
class ApiResponse {
  public:
    ApiResponse() = default;
    explicit ApiResponse(T data) : data_(std::move(data)), success_(true) {}
    explicit ApiResponse(const ApiError& error) : error_(error), success_(false) {}

    bool is_success() const { return success_; }
    bool is_error() const { return !success_; }

    const T& data() const { return data_; }
    T& data() { return data_; }

    const Optional<ApiError>& error() const { return error_; }

    Json::Value to_json() const {
        if (success_) {
            // For successful responses with data
            if constexpr (std::is_same_v<T, Json::Value>) {
                return data_;
            } else {
                return data_.to_json();
            }
        } else {
            // For error responses
            return error_->to_json();
        }
    }

  private:
    T data_;
    Optional<ApiError> error_;
    bool success_{true};
};

/**
 * @brief Success response with no data
 */
struct SuccessResponse {
    bool success{true};
    String message;

    Json::Value to_json() const {
        Json::Value json;
        json["success"] = success;
        if (!message.empty()) {
            json["message"] = message;
        }
        return json;
    }

    static SuccessResponse ok(const String& message = "") { return SuccessResponse{true, message}; }
};

/**
 * @brief Paginated response
 */
template <typename T>
struct PaginatedResponse {
    Vector<T> items;
    int64_t total_count{0};
    int32_t page{1};
    int32_t page_size{20};
    bool has_more{false};
    Optional<String> continuation_token;

    Json::Value to_json() const {
        Json::Value json;

        Json::Value items_array(Json::arrayValue);
        for (const auto& item : items) {
            if constexpr (std::is_same_v<T, Json::Value>) {
                items_array.append(item);
            } else {
                items_array.append(item.to_json());
            }
        }
        json["items"] = items_array;

        json["total_count"] = Json::Int64(total_count);
        json["page"] = page;
        json["page_size"] = page_size;
        json["has_more"] = has_more;

        if (continuation_token) {
            json["continuation_token"] = *continuation_token;
        }

        return json;
    }
};

/**
 * @brief Upload progress response
 */
struct UploadProgress {
    String upload_id;
    String object_key;
    int64_t bytes_uploaded{0};
    int64_t total_bytes{0};
    double progress_percent{0.0};
    String status; // "in_progress", "completed", "failed"

    Json::Value to_json() const {
        Json::Value json;
        json["upload_id"] = upload_id;
        json["object_key"] = object_key;
        json["bytes_uploaded"] = Json::Int64(bytes_uploaded);
        json["total_bytes"] = Json::Int64(total_bytes);
        json["progress_percent"] = progress_percent;
        json["status"] = status;
        return json;
    }
};

/**
 * @brief Health check response
 */
struct HealthResponse {
    String status; // "ok", "degraded", "unavailable"
    String service{"object-storage-console"};
    String version;
    int64_t timestamp{0};
    StringMap checks; // service_name -> status

    Json::Value to_json() const {
        Json::Value json;
        json["status"] = status;
        json["service"] = service;
        json["version"] = version;
        json["timestamp"] = Json::Int64(timestamp);

        Json::Value checks_obj;
        for (const auto& [name, check_status] : checks) {
            checks_obj[name] = check_status;
        }
        json["checks"] = checks_obj;

        return json;
    }
};

/**
 * @brief Server info response
 */
struct ServerInfoResponse {
    String version;
    String commit_id;
    String build_time;
    String go_version;
    String platform;
    int64_t uptime_seconds{0};
    StringMap features;

    Json::Value to_json() const {
        Json::Value json;
        json["version"] = version;
        json["commit_id"] = commit_id;
        json["build_time"] = build_time;
        json["go_version"] = go_version;
        json["platform"] = platform;
        json["uptime_seconds"] = Json::Int64(uptime_seconds);

        Json::Value features_obj;
        for (const auto& [name, value] : features) {
            features_obj[name] = value;
        }
        json["features"] = features_obj;

        return json;
    }
};

/**
 * @brief Batch operation result
 */
template <typename T>
struct BatchOperationResult {
    Vector<T> succeeded;
    Vector<std::pair<T, String>> failed; // item + error message
    int32_t total_count{0};
    int32_t success_count{0};
    int32_t failure_count{0};

    Json::Value to_json() const {
        Json::Value json;

        Json::Value succeeded_array(Json::arrayValue);
        for (const auto& item : succeeded) {
            if constexpr (std::is_same_v<T, String>) {
                succeeded_array.append(item);
            } else if constexpr (std::is_same_v<T, Json::Value>) {
                succeeded_array.append(item);
            } else {
                succeeded_array.append(item.to_json());
            }
        }
        json["succeeded"] = succeeded_array;

        Json::Value failed_array(Json::arrayValue);
        for (const auto& [item, error] : failed) {
            Json::Value failure;
            if constexpr (std::is_same_v<T, String>) {
                failure["item"] = item;
            } else if constexpr (std::is_same_v<T, Json::Value>) {
                failure["item"] = item;
            } else {
                failure["item"] = item.to_json();
            }
            failure["error"] = error;
            failed_array.append(failure);
        }
        json["failed"] = failed_array;

        json["total_count"] = total_count;
        json["success_count"] = success_count;
        json["failure_count"] = failure_count;

        return json;
    }
};

} // namespace console::models
