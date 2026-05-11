# S3/Object Storage client implementations

set(CLIENTS_SOURCES
    LocalAdminClient.cpp
    LocalStorageClient.cpp
)

add_library(console_clients STATIC ${CLIENTS_SOURCES})

target_include_directories(console_clients
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_clients
    PUBLIC
        console_common
        console_utils
        console_models
        Drogon::Drogon
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)
