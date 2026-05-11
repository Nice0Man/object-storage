#pragma once

#include "console/common/Types.hpp"

namespace console {

bool validate_security_config();

void configure_drogon(const ServerConfig& config);

void setup_cors();

void setup_api_stats_collection();

void setup_http_access_logging();

void setup_background_tasks();

void register_routes();

void print_banner();

} // namespace console
