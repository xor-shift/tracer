#pragma once

#include <stuff/random.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <concepts>
#include <optional>

namespace trc::concepts {

template<typename T>
concept shape =//
  requires(T const& shape, ray const& ray, vec3 const& pt) {
      { shape.intersect(ray) } -> std::convertible_to<std::optional<intersection>>;
      { shape.intersects(ray) } -> std::convertible_to<bool>;
      { shape.normal_at(pt) } -> std::convertible_to<vec3>;
      { shape.material_index() } -> std::convertible_to<u32>;
  };

template<typename T>
concept bound_shape =//
  shape<T> &&
  requires(T const& shape, ray const& ray, stf::random::xoshiro_256p& gen) {
      { shape.bounds() } -> std::convertible_to<std::pair<vec3, vec3>>;
      { shape.center() } -> std::convertible_to<vec3>;
      { shape.sample_surface(gen) } -> std::convertible_to<intersection>;
      { shape.surface_area() } -> std::convertible_to<real>;
  };

template<typename T>
concept unbound_shape = shape<T> && (!bound_shape<T>);

}// namespace trc::concepts
