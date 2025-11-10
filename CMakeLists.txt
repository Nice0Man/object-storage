cmake_minimum_required(VERSION 3.20)
project(ObjectStorageConsole
    VERSION 1.0.0
    DESCRIPTION "OpenMaxIO Object Browser - S3-compatible web console"
    LANGUAGES CXX
)

# ============================================================================
# Project Configuration
# ============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================================
# Build Options
# ============================================================================

option(BUILD_TESTS "Build tests" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_TSAN "Enable Thread Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)

# ============================================================================
# Compiler Flags
# ============================================================================

# Warning flags
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wconversion
        -Wsign-conversion
        -Wno-unused-parameter
    )
endif()

# Optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3 -march=native)
    endif()
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
    if(NOT MSVC)
        add_compile_options(-g -O0)
    endif()
endif()

# Sanitizers
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

# Code coverage
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    endif()
endif()

# ============================================================================
# Dependencies
# ============================================================================

find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# ============================================================================
# Project Structure
# ============================================================================

set(CONSOLE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(CONSOLE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(CONSOLE_TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

# Include directories
include_directories(
    ${CONSOLE_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

# ============================================================================
# Source Files
# ============================================================================

# Core sources
file(GLOB_RECURSE CONSOLE_SOURCES
    ${CONSOLE_SOURCE_DIR}/*.cpp
)

# Remove main.cpp from library sources (it will be in executable)
list(FILTER CONSOLE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# ============================================================================
# Library Target
# ============================================================================

add_library(console_lib STATIC ${CONSOLE_SOURCES})

target_include_directories(console_lib
    PUBLIC
        ${CONSOLE_INCLUDE_DIR}
    PRIVATE
        ${CONSOLE_SOURCE_DIR}
)

target_link_libraries(console_lib
    PUBLIC
        Drogon::Drogon
        nlohmann_json::nlohmann_json
        jwt-cpp::jwt-cpp
        spdlog::spdlog
        OpenSSL::SSL
        OpenSSL::Crypto
        Boost::system
)

target_compile_features(console_lib PUBLIC cxx_std_20)

# ============================================================================
# Executable Target
# ============================================================================

add_executable(console ${CONSOLE_SOURCE_DIR}/main.cpp)

target_link_libraries(console
    PRIVATE
        console_lib
)

# Set output directory
set_target_properties(console PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
# Tests
# ============================================================================

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS console
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/web-app/build/
    DESTINATION share/console/web-app
    OPTIONAL
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:          ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build tests:       ${BUILD_TESTS}")
message(STATUS "  Code coverage:     ${ENABLE_COVERAGE}")
message(STATUS "  Address Sanitizer: ${ENABLE_ASAN}")
message(STATUS "  Thread Sanitizer:  ${ENABLE_TSAN}")
message(STATUS "  UB Sanitizer:      ${ENABLE_UBSAN}")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "")
