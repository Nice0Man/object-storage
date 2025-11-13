# HTTP filters

file(GLOB FILTERS_SOURCES *.cpp)

add_library(console_filters STATIC ${FILTERS_SOURCES})

target_include_directories(console_filters
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(console_filters
    PUBLIC
        console_common
        console_utils
        console_middleware
        Drogon::Drogon
        spdlog::spdlog
)
