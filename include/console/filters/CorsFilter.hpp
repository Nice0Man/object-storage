#pragma once

#include <drogon/HttpFilter.h>
#include <drogon/HttpTypes.h>

namespace console::filters {

class CorsFilter : public drogon::HttpFilter<CorsFilter> {
  public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};

} // namespace console::filters
