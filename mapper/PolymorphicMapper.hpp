#pragma once

#include <optional>

template <class Source, auto value> struct Mapping {
  using TargetType = decltype(value);
  static constexpr auto mapped_value = value;
};

template <class Base, class Target, class... MapEntries>
struct PolymorphicMapper;

template <class Base, class Target>
struct PolymorphicMapper<Base, Target> {
  static std::optional<Target> map(const Base&) { return std::nullopt; }
};

template <class Base, class Target, class... MapEntries> struct MapperImpl;

template <class Base, class Target>
struct MapperImpl<Base, Target> {
  static std::optional<Target> map(const Base&) {
    return PolymorphicMapper<Base, Target>::map({});
  }
};

template <class Base, class Target, class Derived, auto mappedValue,
          class... RemainingMappings>
struct MapperImpl<Base, Target, Mapping<Derived, mappedValue>,
                  RemainingMappings...> {
  static std::optional<Target> map(const Base& instance) {
    if (dynamic_cast<const Derived*>(std::addressof(instance))) {
      return {mappedValue};
    } else {
      return MapperImpl<Base, Target, RemainingMappings...>::map(instance);
    }
  }
};

template <class Base, class Target, class Derived, auto mappedValue,
          class... RemainingMappings>
struct PolymorphicMapper<Base, Target, Mapping<Derived, mappedValue>,
                         RemainingMappings...> {
  static std::optional<Target> map(const Base &instance) {
    return MapperImpl<Base, Target, Mapping<Derived, mappedValue>,
                      RemainingMappings...>::map(instance);
  }
};
