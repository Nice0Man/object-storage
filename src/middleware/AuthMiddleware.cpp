// Copyright 2025 OpenMaxIO Contributors
// Licensed under AGPL-3.0
//
#include "console/middleware/AuthMiddleware.hpp"

#include "console/common/Config.hpp"
#include "console/common/Logger.hpp"

#include <json/json.h>
#include <jwt-cpp/jwt.h>

namespace console::middleware {

void
AuthMiddleware::doFilter(const drogon::HttpRequestPtr& req,
                         drogon::FilterCallback&& fcb,
                         drogon::FilterChainCallback&& fccb) {
    // Extract token from request
    auto token = extract_token(req);

    if (!token.has_value()) {
        LOG_WARN("No authentication token provided");

        Json::Value error;
        error["error"] = "Authentication required";
        error["message"] = "No token provided";

        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k401Unauthorized);
        fcb(resp);
        return;
    }

    // Validate token
    if (!validate_token(*token)) {
        LOG_WARN("Invalid authentication token");

        Json::Value error;
        error["error"] = "Invalid token";
        error["message"] = "Token validation failed";

        auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(drogon::k401Unauthorized);
        fcb(resp);
        return;
    }

    // Token is valid, continue to next filter/handler
    fccb();
}

bool
AuthMiddleware::validate_token(const String& token) const {
    try {
        auto& config = Config::instance();
        auto& auth_config = config.auth();

        auto decoded = jwt::decode(token);

        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{auth_config.jwt_secret})
                            .with_issuer("object-storage-console");

        verifier.verify(decoded);

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Token validation error: {}", e.what());
        return false;
    }
}

Optional<String>
AuthMiddleware::extract_token(const drogon::HttpRequestPtr& req) const {
    // Check Authorization header
    auto auth_header = req->getHeader("Authorization");
    if (!auth_header.empty()) {
        // Expected format: "Bearer <token>"
        const String bearer_prefix = "Bearer ";
        if (auth_header.find(bearer_prefix) == 0) {
            return auth_header.substr(bearer_prefix.length());
        }
    }

    // Check query parameter as fallback
    auto token_param = req->getParameter("token");
    if (!token_param.empty()) {
        return String(token_param);
    }

    return std::nullopt;
}

} // namespace console::middleware
