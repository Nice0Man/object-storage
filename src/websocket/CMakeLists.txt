# WebSocket controllers

file(GLOB WEBSOCKET_SOURCES *.cpp)

add_library(console_websocket STATIC ${WEBSOCKET_SOURCES})

target_include_directories(console_websocket
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_websocket
    PUBLIC
        console_common
        console_utils
        console_services
        Drogon::Drogon
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)
