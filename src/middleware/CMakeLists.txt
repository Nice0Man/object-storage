# HTTP middleware components

set(MIDDLEWARE_SOURCES
    ErrorHandler.cpp
    RateLimiter.cpp
    RequestLogger.cpp
)

add_library(console_middleware STATIC ${MIDDLEWARE_SOURCES})

target_include_directories(console_middleware
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_middleware
    PUBLIC
        console_common
        console_utils
        Drogon::Drogon
        spdlog::spdlog
)
