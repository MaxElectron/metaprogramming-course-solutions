#pragma once

#include <optional>
#include <type_traits>

template <class Source, auto value>
struct Mapping {
    using TargetType = std::remove_cv_t<decltype(value)>;
    static constexpr auto mapped_value = value;
};

template <class Base, class Target, class... MapEntries>
struct MapperImpl;

template <class Base, class Target, class... MapEntries>
struct PolymorphicMapper {
  static std::optional<Target> map(const Base& instance) {
    return MapperImpl<Base, Target, MapEntries...>::processMapping(instance);
  }
};

template <class Base, class Target, class Derived, auto mappedValue, class... RemainingMappings>
struct MapperImpl<Base, Target, Mapping<Derived, mappedValue>, RemainingMappings...> {
    static_assert(std::is_base_of_v<Base, Derived>);
    
  static std::optional<Target> processMapping(const Base& instance) {
    if constexpr (std::is_same_v<std::remove_cv_t<decltype(mappedValue)>, Target>) {
          if (dynamic_cast<const Derived*>(std::addressof(instance)) != nullptr) {
            return {mappedValue};
          } else {
            if constexpr (sizeof...(RemainingMappings) > 0) {
              return MapperImpl<Base, Target, RemainingMappings...>::processMapping(instance);
            } else {
              return std::nullopt;
            }
        }
    }
    else {
        return std::nullopt;
    }
  }
};

template <class Base, class Target>
struct MapperImpl<Base, Target> {
  static std::optional<Target> processMapping(const Base&) {
    return std::nullopt;
  }
};

template <class Base, class Target>
struct PolymorphicMapper<Base, Target> {
    static std::optional<Target> map(const Base&) {
        return std::nullopt;
    }
};
