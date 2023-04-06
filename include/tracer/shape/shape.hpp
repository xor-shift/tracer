#pragma once

#include <stuff/random.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <concepts>
#include <optional>

namespace trc::concepts {

template<typename T>
concept shape =//
  requires(T const& shape, ray const& ray, vec3 const& pt, real t, pixel_statistics& stats) {
      { shape.intersect(ray, t) } -> std::convertible_to<std::optional<intersection>>;
      { shape.intersect(ray) } -> std::convertible_to<std::optional<intersection>>;
      { shape.intersect(ray, stats, t) } -> std::convertible_to<std::optional<intersection>>;
      { shape.intersect(ray, stats) } -> std::convertible_to<std::optional<intersection>>;
      { shape.intersects(ray) } -> std::convertible_to<bool>;
      // { shape.normal_at(pt) } -> std::convertible_to<vec3>;
      // { shape.material_index() } -> std::convertible_to<u32>;
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

namespace trc {

// intended to be used with BVH trees
struct dyn_shape {
    virtual ~dyn_shape() = default;

    virtual auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection> = 0;

    virtual auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> = 0;

    virtual auto intersects(ray const& ray) const -> bool = 0;
};

static_assert(concepts::shape<dyn_shape>);

}// namespace trc
