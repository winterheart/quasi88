/* SPDX-License-Identifier: BSD-3-Clause */

#include <gtest/gtest.h>

#include "byteswap.h"

TEST(Endianess, CheckConversion) {
  EXPECT_EQ(QUASI88::byteswap((uint8_t)0x01), (uint8_t)0x01);
  EXPECT_EQ(QUASI88::byteswap((int8_t)0x01), (int16_t)0x01);
  EXPECT_EQ(QUASI88::byteswap((uint16_t)0x0123), (uint16_t)0x2301);
  EXPECT_EQ(QUASI88::byteswap((int16_t)0x0123), (int16_t)0x2301);
  EXPECT_EQ(QUASI88::byteswap((int32_t)0x01234567), (int32_t)0x67452301);
  EXPECT_EQ(QUASI88::byteswap((int64_t)0x0123456789ABCDEF), (int64_t)0xEFCDAB8967452301);
#ifdef LSB_FIRST
  EXPECT_EQ(QUASI88::convert_le((int16_t)0x10), 0x10);
  EXPECT_EQ(QUASI88::convert_le((int32_t)0x10), 0x10);
  EXPECT_EQ(QUASI88::convert_le((int64_t)0x10), 0x10);

  EXPECT_EQ(QUASI88::convert_be((int16_t)0x10), 0x1000);
  EXPECT_EQ(QUASI88::convert_be((int32_t)0x10), 0x10000000);
  EXPECT_EQ(QUASI88::convert_be((int64_t)0x10), 0x1000000000000000);
#else
  EXPECT_EQ(QUASI88::convert_be((int16_t)0x10), 0x10);
  EXPECT_EQ(QUASI88::convert_be((int32_t)0x10), 0x10);
  EXPECT_EQ(QUASI88::convert_be((int64_t)0x10), 0x10);

  EXPECT_EQ(QUASI88::convert_le((int16_t)0x10), 0x1000);
  EXPECT_EQ(QUASI88::convert_le((int16_t)0x10), 0x10000000);
  EXPECT_EQ(QUASI88::convert_le((int16_t)0x10), 0x1000000000000000);
#endif
}
