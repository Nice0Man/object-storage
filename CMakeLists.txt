cmake_minimum_required(VERSION 3.20)
project(ObjectStorageConsole
    VERSION 1.0.0
    DESCRIPTION "Object Storage Console - S3-compatible web console"
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
option(ENABLE_LTO "Enable Link Time Optimization" ON)
option(ENABLE_UNITY_BUILD "Enable Unity/Jumbo builds" ON)
option(ENABLE_STRIP "Strip symbols in Release" ON)

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
        # -Werror  # Disabled to allow external library warnings
        -Wconversion
        -Wsign-conversion
        -Wno-unused-parameter
    )
endif()

# Optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2 /GL)
        add_link_options(/LTCG)
    else()
        add_compile_options(
            -O3
            -march=native
            -mtune=native
            -ffast-math
            -funroll-loops
            -fomit-frame-pointer
        )
    endif()

    # Link Time Optimization
    if(ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT lto_supported OUTPUT lto_error)

        if(lto_supported)
            message(STATUS "LTO/IPO enabled")
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message(WARNING "LTO/IPO not supported: ${lto_error}")
        endif()
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
find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# jwt-cpp is header-only, fetch from GitHub if not found
find_package(jwt-cpp CONFIG QUIET)
if(NOT jwt-cpp_FOUND)
    message(STATUS "jwt-cpp not found, fetching from GitHub...")
    include(FetchContent)

    FetchContent_Declare(
        jwt-cpp
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
        GIT_TAG v0.7.1
        GIT_SHALLOW TRUE
    )

    set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(JWT_BUILD_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(jwt-cpp)

    message(STATUS "jwt-cpp fetched successfully")
endif()

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

# Unity/Jumbo builds for faster compilation
if(ENABLE_UNITY_BUILD)
    set_target_properties(console_lib PROPERTIES
        UNITY_BUILD ON
        UNITY_BUILD_BATCH_SIZE 16
    )
endif()

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

# Disable warnings for external libraries
if(TARGET jwt-cpp)
    target_compile_options(jwt-cpp INTERFACE -w)
endif()

# Position Independent Code for better security and performance
set_target_properties(console_lib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# ============================================================================
# Executable Target
# ============================================================================

add_executable(console ${CONSOLE_SOURCE_DIR}/main.cpp)

target_link_libraries(console
    PRIVATE
        console_lib
)

# Set output directory and properties
set_target_properties(console PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Strip symbols in Release builds for smaller binary
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ENABLE_STRIP AND NOT MSVC)
    add_custom_command(TARGET console POST_BUILD
        COMMAND ${CMAKE_STRIP} --strip-unneeded $<TARGET_FILE:console>
        COMMENT "Stripping symbols from release binary"
    )
endif()

# Link everything statically for single binary deployment (optional)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(NOT MSVC AND NOT APPLE)
        target_link_options(console PRIVATE
            -static-libgcc
            -static-libstdc++
        )
    endif()
endif()

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
message(STATUS "  LTO/IPO:           ${ENABLE_LTO}")
message(STATUS "  Unity builds:      ${ENABLE_UNITY_BUILD}")
message(STATUS "  Strip symbols:     ${ENABLE_STRIP}")
message(STATUS "  Code coverage:     ${ENABLE_COVERAGE}")
message(STATUS "  Address Sanitizer: ${ENABLE_ASAN}")
message(STATUS "  Thread Sanitizer:  ${ENABLE_TSAN}")
message(STATUS "  UB Sanitizer:      ${ENABLE_UBSAN}")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "")
