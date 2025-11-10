#include "console/common/Config.hpp"

#include <gtest/gtest.h>

using namespace console;

class ConfigTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup test configuration
    }

    void TearDown() override {
        // Cleanup
    }
};

TEST_F(ConfigTest, LoadFromString) {
    auto& config = Config::instance();

    String json_config = R"({
        "server": {
            "host": "127.0.0.1",
            "port": 8080
        }
    })";

    ASSERT_TRUE(config.load_from_string(json_config));
    EXPECT_EQ(config.server().host, "127.0.0.1");
    EXPECT_EQ(config.server().port, 8080);
}

TEST_F(ConfigTest, DefaultValues) {
    auto& config = Config::instance();
    config.load_from_string("{}");

    EXPECT_EQ(config.server().host, "0.0.0.0");
    EXPECT_EQ(config.server().port, 9090);
}

TEST_F(ConfigTest, InvalidJson) {
    auto& config = Config::instance();
    String invalid_json = "{ invalid json }";

    EXPECT_FALSE(config.load_from_string(invalid_json));
    EXPECT_FALSE(config.is_valid());
}
