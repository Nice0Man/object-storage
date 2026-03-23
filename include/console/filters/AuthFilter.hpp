#pragma once

#include "console/common/Config.hpp"
#include "console/common/Types.hpp"
#include "console/utils/JWT.hpp"

#include <drogon/HttpFilter.h>

#include <memory>

/**
 * @brief Authentication filter (global namespace wrapper)
 *
 * Drogon requires filters to be in global namespace for automatic registration.
 * This wraps the console::middleware::AuthMiddleware logic.
 */
class AuthFilter : public drogon::HttpFilter<AuthFilter> {
  public:
    AuthFilter() { CONSOLE_LOG_INFO("AuthFilter registered"); }

    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override {
        if (req->method() == drogon::HttpMethod::Options) {
            fccb();
            return;
        }

        auto add_cors_headers = [&req](const drogon::HttpResponsePtr& resp) {
            auto origin = req->getHeader("Origin");
            if (origin.empty()) {
                origin = "http://localhost:3000";
            }
            resp->addHeader("Access-Control-Allow-Origin", origin);
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
            resp->addHeader("Access-Control-Allow-Headers",
                            "Content-Type, Authorization, X-Requested-With, Accept, Origin");
            resp->addHeader("Access-Control-Allow-Credentials", "true");
        };

        auto auth_header = req->getHeader("Authorization");
        if (auth_header.empty() || auth_header.find("Bearer ") != 0) {
            CONSOLE_LOG_WARN("No authentication token provided for {}", req->getPath());
            Json::Value error;
            error["error"] = "Unauthorized";
            error["message"] = "No token provided";
            error["code"] = 401;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            add_cors_headers(resp);
            fcb(resp);
            return;
        }

        console::String token = auth_header.substr(7);

        auto claims_result = console::utils::JWT::validate_token(token);
        if (claims_result.is_err()) {
            CONSOLE_LOG_WARN("Invalid or expired token for {}: {}", req->getPath(), claims_result.error());
            Json::Value error;
            error["error"] = "Unauthorized";
            error["message"] = "Invalid or expired token";
            error["code"] = 401;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(error);
            resp->setStatusCode(drogon::k401Unauthorized);
            add_cors_headers(resp);
            fcb(resp);
            return;
        }

        auto& claims = claims_result.value();
        console::UserInfo user_info = console::utils::JWT::claims_to_userinfo(claims);

        req->attributes()->insert("user_info", user_info);

        CONSOLE_LOG_DEBUG("AuthFilter: Stored user_info for '{}', is_admin={} at path {}",
                          user_info.access_key,
                          user_info.is_admin,
                          req->getPath());

        fccb();
    }
};
