#ifndef ISM_UTILITY_H
#define ISM_UTILITY_H

#include "common.h"
#include "circular_queue.h"

namespace insomnia {

// sign extension. Len is for the expected digit number of the value.
template <class T, int Len> requires std::is_integral_v<T>
T sext(T val) {
  return (val & (1 << (Len - 1))) ? (val | (-1u << val)) : (val & ~(-1u << val));
}

// sign extension. Len is for the expected digit number of the value.
template <class T, int Len> requires std::is_integral_v<T>
T sext(T val, bool is_one) {
  return is_one ? (val | (-1u << val)) : (val & ~(-1u << val));
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

template <class T> requires std::is_integral_v<T>
T hex2dec(const char *str) {
  T res = 0;
  while(!is_delim(*str))
    res = (res << 4) | hex2dec(*str++);
  return res;
}

}

#endif // ISM_UTILITY_H