# REST API controllers

file(GLOB API_SOURCES *.cpp)

add_library(console_api STATIC ${API_SOURCES})

target_include_directories(console_api
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_api
    PUBLIC
        console_common
        console_utils
        console_models
        console_services
        Drogon::Drogon
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)
