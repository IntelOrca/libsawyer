#include <gtest/gtest.h>
#include <sawyer/String.h>

TEST(StringTests, equals)
{
    ASSERT_EQ(cs::equals("abc", "abc"), true);
    ASSERT_EQ(cs::equals("abc", "Abc"), false);
    ASSERT_EQ(cs::equals("abc", "cba"), false);
    ASSERT_EQ(cs::equals("abc", "ab"), false);
    ASSERT_EQ(cs::equals("abc", "abc", true), true);
    ASSERT_EQ(cs::equals("abc", "Abc", true), true);
    ASSERT_EQ(cs::equals("abc", "cba", true), false);
    ASSERT_EQ(cs::equals("abc", "ab", true), false);
}

TEST(StringTests, iequals)
{
    ASSERT_EQ(cs::iequals("abc", "abc"), true);
    ASSERT_EQ(cs::iequals("abc", "Abc"), true);
    ASSERT_EQ(cs::iequals("abc", "cba"), false);
    ASSERT_EQ(cs::iequals("abc", "ab"), false);
}

TEST(StringTests, logicalcmp)
{
    auto res_logical_1 = cs::logicalcmp("foo1", "foo1_2");
    auto res_logical_2 = cs::logicalcmp("foo1_2", "foo1");
    auto res_1 = strcmp("foo1", "foo1_2");
    auto res_2 = strcmp("foo1_2", "foo1");
    // We only care if sign is correct, actual values might not be.
    EXPECT_GE(res_1 * res_logical_1, 1);
    EXPECT_GE(res_2 * res_logical_2, 1);
    EXPECT_NE(res_logical_1, res_logical_2);

    EXPECT_GT(cs::logicalcmp("foo12", "foo1"), 0);
    EXPECT_LT(cs::logicalcmp("foo12", "foo13"), 0);
    EXPECT_EQ(cs::logicalcmp("foo13", "foo13"), 0);

    EXPECT_EQ(cs::logicalcmp("foo13", "FOO13"), 0);

    EXPECT_LT(cs::logicalcmp("A", "b"), 0);
    EXPECT_LT(cs::logicalcmp("a", "B"), 0);
    EXPECT_GT(cs::logicalcmp("B", "a"), 0);
    EXPECT_GT(cs::logicalcmp("b", "A"), 0);

    // ^ is used at the start of a ride name in OpenRCT2 to move it to the end of the list
    EXPECT_LT(cs::logicalcmp("A", "^"), 0);
    EXPECT_LT(cs::logicalcmp("a", "^"), 0);
    EXPECT_LT(cs::logicalcmp("!", "A"), 0);
    EXPECT_LT(cs::logicalcmp("!", "a"), 0);
}

TEST(StringTests, toUtf8)
{
    ASSERT_EQ(cs::toUtf8(L"abc"), "abc");
    ASSERT_EQ(cs::toUtf8(L"列車"), u8"列車");
}

TEST(StringTests, toUtf16)
{
    ASSERT_EQ(cs::toUtf16("abc"), L"abc");
    ASSERT_EQ(cs::toUtf16(u8"列車"), L"列車");
}
