# CLI module

file(GLOB CLI_SOURCES *.cpp)

add_library(console_cli STATIC ${CLI_SOURCES})

target_include_directories(console_cli
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_cli
    PUBLIC
        console_common
        console_services
        console_storage
        console_clients
        console_utils
        spdlog::spdlog
)
