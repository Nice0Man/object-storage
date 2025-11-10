cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test Configuration
# ============================================================================

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

enable_testing()

# ============================================================================
# Test Sources
# ============================================================================

set(UNIT_TEST_SOURCES
    unit/ConfigTest.cpp
    unit/LoggerTest.cpp
    unit/TypesTest.cpp
    unit/JWTTest.cpp
    unit/ModelsTest.cpp
    unit/ServicesTest.cpp
)

set(INTEGRATION_TEST_SOURCES
    integration/ApiTest.cpp
    integration/AuthTest.cpp
)

# ============================================================================
# Unit Tests
# ============================================================================

add_executable(unit_tests ${UNIT_TEST_SOURCES})

target_link_libraries(unit_tests
    PRIVATE
        console_lib
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests
)

gtest_discover_tests(unit_tests)

# ============================================================================
# Integration Tests
# ============================================================================

add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})

target_link_libraries(integration_tests
    PRIVATE
        console_lib
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(integration_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests
)

gtest_discover_tests(integration_tests)

# ============================================================================
# Test Configuration
# ============================================================================

# Set test working directory
set_tests_properties(
    unit_tests
    integration_tests
    PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "Tests configured:")
message(STATUS "  Unit tests:        ${UNIT_TEST_SOURCES}")
message(STATUS "  Integration tests: ${INTEGRATION_TEST_SOURCES}")
