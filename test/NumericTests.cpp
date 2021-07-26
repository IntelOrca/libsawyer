#include <gtest/gtest.h>
#include <sawyer/Numeric.h>

TEST(NumericTests, bitScanForward)
{
    ASSERT_EQ(cs::bitScanForward(0), -1);
    ASSERT_EQ(cs::bitScanForward(0b0001), 0);
    ASSERT_EQ(cs::bitScanForward(0b0010), 1);
    ASSERT_EQ(cs::bitScanForward(0b1100), 2);
    ASSERT_EQ(cs::bitScanForward(0b01000000000000000000000000000000), 30);
    ASSERT_EQ(cs::bitScanForward(0b10000000000000000000000000000000), 31);
}

TEST(NumericTests, bitScanReverse)
{
    ASSERT_EQ(cs::bitScanReverse(0), -1);
    ASSERT_EQ(cs::bitScanReverse(0b00100000000000000000000000000100), 29);
    ASSERT_EQ(cs::bitScanReverse(0b01000000000000000000000000001000), 30);
    ASSERT_EQ(cs::bitScanReverse(0b10000000000000000000000000000100), 31);
    ASSERT_EQ(cs::bitScanReverse(0b0001), 0);
    ASSERT_EQ(cs::bitScanReverse(0b0010), 1);
    ASSERT_EQ(cs::bitScanReverse(0b1100), 3);
}
