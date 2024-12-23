#pragma once

#include <algorithm>
#include <array>
#include <string_view>

template <size_t max_len> struct FixedString {
  std::array<char, max_len + 1> data{};
  size_t length;

  constexpr FixedString(const char *input_str, size_t input_len) noexcept
      : length(input_len) {
    std::ranges::copy(input_str, input_str + input_len, data.begin());
  }

  constexpr operator std::string_view() const noexcept {
    return std::string_view(data.data(), length);
  }
};

constexpr FixedString<256> operator"" _cstr(const char *literal_str,
                                            size_t literal_len) noexcept {
  return FixedString<256>(literal_str, literal_len);
}
