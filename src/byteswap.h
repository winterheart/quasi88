/* SPDX-FileCopyrightText: 2023 Azamat H. Hackimov <azamat.hackimov@gmail.com> */
/* SPDX-License-Identifier: BSD-3-Clause */
#pragma once

namespace QUASI88 {

// std::byteswap from C++23
template <typename T> constexpr T byteswap(T n) {
  T m;
  for (size_t i = 0; i < sizeof(T); i++)
    reinterpret_cast<uint8_t *>(&m)[i] = reinterpret_cast<uint8_t *>(&n)[sizeof(T) - 1 - i];
  return m;
}

/**
 * Convert integer to/from BE order
 */
template <typename T> constexpr T convert_be(T val) {
#ifdef LSB_FIRST
  return byteswap(val);
#else
  return (val);
#endif
}

/**
 * Convert integer to/from LE order
 */
template <typename T> constexpr T convert_le(T val) {
#ifdef LSB_FIRST
  return (val);
#else
  return byteswap(val);
#endif
}

}
