#pragma once

#include <tracer/intersection.hpp>

#include <concepts>

namespace trc::concepts {

template<typename T>
concept material =//
  requires(T const& material, intersection const& isection) {
      //{ material.sample(isection) } -> std::convertible_to<material_sample>;
      true;
  };

}
