
//
#include "console/common/Logger.hpp"

#include <gtest/gtest.h>

using namespace console;

class LoggerTest : public ::testing::Test {
  protected:
    void SetUp() override { Logger::instance().init(LogLevel::Debug, "", 0, 0); }
};

TEST_F(LoggerTest, LogLevels) {
    auto& logger = Logger::instance();

    logger.set_level(LogLevel::Info);
    EXPECT_EQ(logger.get_level(), LogLevel::Info);

    logger.set_level(LogLevel::Debug);
    EXPECT_EQ(logger.get_level(), LogLevel::Debug);
}

TEST_F(LoggerTest, LogMessages) {
    // Test that logging doesn't crash
    CONSOLE_LOG_INFO("Test info message");
    CONSOLE_LOG_DEBUG("Test debug message: {}", 42);
    CONSOLE_LOG_WARN("Test warning");
    CONSOLE_LOG_ERROR("Test error");
}
