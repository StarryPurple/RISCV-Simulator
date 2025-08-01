#ifndef ISM_UTILITY_H
#define ISM_UTILITY_H

#include "common.h"
#include "circular_queue.h"

namespace insomnia {

// sign extension. Len is for the expected digit number of the value.
template <std::integral T, int Len>
T sign_extend(T val) {
  return (val & (1 << (Len - 1))) ? (val | (-1u << Len)) : (val & ~(-1u << Len));
}

// sign extension. Len is for the expected digit number of the value.
template <std::integral T, int Len>
T sign_extend(T val, bool is_one) {
  return is_one ? (val | (-1u << Len)) : (val & ~(-1u << Len));
}

inline bool is_delim(char c) {
  return c == '\0' || c == EOF || c == ' ' || c == '\n' || c == '\r';
}

inline int hex2dec(char c) {
  if('0' <= c && c <= '9') return c - '0';
  if('a' <= c && c <= 'f') return c - 'a' + 10;
  if('A' <= c && c <= 'F') return c - 'A' + 10;
  throw std::runtime_error("invalid hex character.");
}

template <std::integral To, int High, int Low, std::integral From>
requires
  (High >= Low) &&
  (High < std::numeric_limits<From>::digits) &&
  (Low >= 0) &&
  (High - Low + 1 <= std::numeric_limits<To>::digits)
To slice_bytes(From val) {
  constexpr From mask = ((static_cast<From>(1) << (High - Low + 1)) - 1) << Low;
  return static_cast<To>((val & mask) >> Low);
}

inline uint32_t ToSmallEndian32_8(uint32_t bigEndian) {
  auto s7_0 = slice_bytes<uint32_t, 31, 24>(bigEndian);
  auto s15_8 = slice_bytes<uint32_t, 23, 16>(bigEndian);
  auto s23_16 = slice_bytes<uint32_t, 15, 8>(bigEndian);
  auto s31_24 = slice_bytes<uint32_t, 7, 0>(bigEndian);
  return (s31_24 << 24) | (s23_16 << 16) | (s15_8 << 8) | (s7_0 << 0);
}

/*
// concatenate the lower digits of two values
template <std::integral To, int Len1, int Len2, std::integral Left, std::integral Right>
requires
  (Len1 >= 0) && (Len2 >= 0) &&
  (Len1 <= std::numeric_limits<Left>::digits) &&
  (Len2 <= std::numeric_limits<Right>::digits) &&
  (Len1 + Len2 <= std::numeric_limits<To>::digits)
To concat_bytes(Left left, Right right) {
  constexpr Left left_mask = (static_cast<Left>(1) << Len1) - 1;
  constexpr Right right_mask = (static_cast<Right>(1) << Len2) - 1;
  return (static_cast<To>(left & left_mask) << Len2) |
          static_cast<To>(right & right_mask);
}
*/

}

#endif // ISM_UTILITY_H