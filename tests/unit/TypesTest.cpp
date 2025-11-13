
//
#include "console/common/Types.hpp"

#include <gtest/gtest.h>

using namespace console;

TEST(TypesTest, ResultOk) {
    Result<int, String> result = Ok<int, String>(42);

    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_err());
    EXPECT_EQ(result.value(), 42);
}

TEST(TypesTest, ResultError) {
    Result<int, String> result = Err<int, String>(String("error message"));

    EXPECT_FALSE(result.is_ok());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error(), "error message");
}

TEST(TypesTest, ResultMap) {
    Result<int, String> result = Ok<int, String>(42);

    auto mapped = result.map([](int x) { return x * 2; });

    EXPECT_TRUE(mapped.is_ok());
    EXPECT_EQ(mapped.value(), 84);
}

TEST(TypesTest, ResultValueOr) {
    Result<int, String> result_ok = Ok<int, String>(42);
    Result<int, String> result_err = Err<int, String>(String("error"));

    EXPECT_EQ(result_ok.value_or(0), 42);
    EXPECT_EQ(result_err.value_or(0), 0);
}
