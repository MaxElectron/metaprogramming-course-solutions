#pragma once

#include <algorithm>
#include <string_view>

template <typename Enum, std::size_t MaxRange = 512>
  requires std::is_enum_v<Enum>
struct EnumeratorTraits {
private:
  using EnumType = Enum;
  using UnderlyingType = std::underlying_type_t<Enum>;

  static constexpr auto upperBound_ = static_cast<unsigned long long>(
      std::numeric_limits<UnderlyingType>::max());
  static constexpr auto lowerBound_ =
      std::numeric_limits<UnderlyingType>::min();
  static constexpr std::size_t rangeSize_ =
      std::min(upperBound_, static_cast<unsigned long long>(MaxRange));

  static constexpr long long signedLowerBound_ = [] {
    if constexpr (std::is_signed_v<UnderlyingType>) {
      return std::max(static_cast<long long>(lowerBound_),
                      static_cast<long long>(-MaxRange));
    } else {
      return 0LL;
    }
  }();

  struct EnumR {
    template <auto Val> static constexpr auto valueFunc() {
      return __PRETTY_FUNCTION__;
    }

    template <typename E, auto Idx> static constexpr E idxToValue() {
      return static_cast<E>(Idx);
    }

    template <typename E, long long Idx> static constexpr bool isValidIndex() {
      const std::string_view str = valueFunc<idxToValue<E, Idx>()>();
      return str[str.rfind('=') + 2] != '(';
    }

    template <typename E, long long Idx>
    static consteval std::string_view extractName() {
      std::string_view str = valueFunc<idxToValue<E, Idx>()>();
      std::size_t pos = str.rfind('=') + 2;
      str = str.substr(pos, str.size() - pos - 1);
      pos = str.find_last_of(':') + 1;
      if (pos != std::string_view::npos) {
        str = str.substr(pos, str.size() - pos);
      }
      return str;
    }
  };

  static consteval std::size_t computeEnumSize() {
    std::size_t count = 0;
    if constexpr (std::is_signed_v<UnderlyingType> && signedLowerBound_ < 0) {
      [&count]<std::size_t... Idx>(std::integer_sequence<std::size_t, Idx...>) {
        (
            [&count]() {
              if (EnumR::template isValidIndex<EnumType,
                                               static_cast<long long>(Idx) +
                                                   signedLowerBound_>()) {
                ++count;
              }
            }(),
            ...);
      }(std::make_integer_sequence<std::size_t, static_cast<std::size_t>(
                                                    -signedLowerBound_)>());
    }
    [&count]<std::size_t... Idx>(std::integer_sequence<std::size_t, Idx...>) {
      (
          [&count]() {
            if (EnumR::template isValidIndex<EnumType,
                                             static_cast<long long>(Idx)>()) {
              ++count;
            }
          }(),
          ...);
    }(std::make_integer_sequence<std::size_t, rangeSize_ + 1>());
    return count;
  }

  template <auto Func> static consteval auto computeEnumData() {
    constexpr std::size_t enumSize_ = computeEnumSize();
    std::array<decltype(Func.template operator()<EnumType, 0>()), enumSize_>
        storage {};
    std::size_t idx = 0;

    if constexpr (std::is_signed_v<UnderlyingType> && signedLowerBound_ < 0) {
      [&storage,
       &idx]<std::size_t... I>(std::integer_sequence<std::size_t, I...>) {
        (
            [&storage, &idx]() {
              if (EnumR::template isValidIndex<EnumType,
                                               static_cast<long long>(I) +
                                                   signedLowerBound_>()) {
                storage[idx++] = Func.template operator()<
                    EnumType, static_cast<long long>(I) + signedLowerBound_>();
              }
            }(),
            ...);
      }(std::make_integer_sequence<std::size_t, static_cast<std::size_t>(
                                                    -signedLowerBound_)>());
    }

    [&storage,
     &idx]<std::size_t... I>(std::integer_sequence<std::size_t, I...>) {
      (
          [&storage, &idx]() {
            if (EnumR::template isValidIndex<EnumType,
                                             static_cast<long long>(I)>()) {
              storage[idx++] =
                  Func.template
                  operator()<EnumType, static_cast<long long>(I)>();
            }
          }(),
          ...);
    }(std::make_integer_sequence<std::size_t, rangeSize_ + 1>());

    return storage;
  }

  static constexpr auto valueFunc_ = []<typename E, auto I_>() {
    return EnumR::template idxToValue<E, I_>();
  };

  static constexpr auto valueData_ = computeEnumData<valueFunc_>();

  static constexpr auto nameFunc_ = []<typename E, long long I_>() {
    return EnumR::template extractName<E, I_>();
  };

  static constexpr auto nameData_ = computeEnumData<nameFunc_>();

public:
  static constexpr EnumType at(std::size_t i) noexcept { return valueData_[i]; }

  static constexpr std::size_t size() noexcept { return valueData_.size(); }

  static constexpr std::string_view nameAt(std::size_t i) noexcept {
    return nameData_[i];
  }
};