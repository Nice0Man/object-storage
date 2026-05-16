#pragma once

#include "console/common/Types.hpp"
#include "console/utils/JWT.hpp"

#include <drogon/HttpRequest.h>

namespace console::websocket {

inline bool
authenticate_websocket_request(const drogon::HttpRequestPtr& req, UserInfo& user_info) {
    String token = req->getParameter("token");
    if (token.empty()) {
        auto auth_header = req->getHeader("Authorization");
        if (auth_header.find("Bearer ") == 0) {
            token = auth_header.substr(7);
        }
    }
    if (token.empty()) {
        return false;
    }

    auto claims_result = utils::JWT::validate_token(token);
    if (claims_result.is_err()) {
        return false;
    }

    user_info = utils::JWT::claims_to_userinfo(claims_result.value());
    return true;
}

} // namespace console::websocket
