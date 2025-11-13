# HTTP middleware components

file(GLOB MIDDLEWARE_SOURCES *.cpp)

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
