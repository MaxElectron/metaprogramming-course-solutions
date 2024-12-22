#pragma once

#include <iostream>
#include <optional>
#include <type_traits>

template <class Source, auto value> struct Mapping {
  using TargetType = decltype(value);
  static constexpr auto mapped_value = value;
};

template <class T> struct TypeWrapper {
  using type = T;
};

template <class T> using WrappedType = typename TypeWrapper<T>::type;

template <class Base, class Target, class... MapEntries>
struct PolymorphicMapper;

template <class Base, class Target> struct PolymorphicMapper<Base, Target> {
  static std::optional<Target> map(const Base &) { return std::nullopt; }
};

template <class Base, class Target, class... MapEntries> struct MapperImpl;

template <class Base, class Target, class Derived, auto mappedValue,
          class... RemainingMappings>
  requires std::is_base_of_v<Base, Derived>
struct MapperImpl<Base, Target, Mapping<Derived, mappedValue>,
                  RemainingMappings...> {
  static std::optional<WrappedType<Target>>
  processMapping(const Base &instance) {
    if (dynamic_cast<const Derived *>(std::addressof(instance)) != nullptr) {
      return {mappedValue};
    } else {
      return MapperImpl<Base, Target, RemainingMappings...>::processMapping(
          instance);
    }
  }
};

template <class Base, class Target> struct MapperImpl<Base, Target> {
  static std::optional<WrappedType<Target>>
  processMapping(const Base &instance) {
    return PolymorphicMapper<Base, Target>::map(instance);
  }
};

template <class Base, class Target, class Derived, auto mappedValue,
          class... RemainingMappings>
  requires std::is_base_of_v<Base, Derived>
struct PolymorphicMapper<Base, Target, Mapping<Derived, mappedValue>,
                         RemainingMappings...> {
  static std::optional<Target> map(const Base &instance) {
    return MapperImpl<Base, Target, Mapping<Derived, mappedValue>,
                      RemainingMappings...>::processMapping(instance);
  }
};
