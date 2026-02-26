#pragma once

#include <type_traits>

namespace kai {

template <typename Derived_Reference, typename Base>
Derived_Reference derived_cast(Base &base) {
  using Derived = std::remove_reference_t<Derived_Reference>;
  static_assert(std::is_base_of_v<Base, Derived>);
  static_assert(!std::is_base_of_v<Derived, Base>);
  return static_cast<Derived_Reference>(base);
}

}  // namespace kai
