#pragma once

#include <stuff/random.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <concepts>
#include <optional>

namespace trc::concepts {

template<typename T>
concept shape =//
  requires(T const& shape, T& mut_shape, ray const& ray, vec3 const& pt, real t, pixel_statistics& stats) {
      { shape.intersect(ray, t) } -> std::convertible_to<std::optional<intersection>>;
      { shape.intersect(ray) } -> std::convertible_to<std::optional<intersection>>;
      { shape.intersect(ray, stats, t) } -> std::convertible_to<std::optional<intersection>>;
      { shape.intersect(ray, stats) } -> std::convertible_to<std::optional<intersection>>;
      { shape.intersects(ray) } -> std::convertible_to<bool>;
      { mut_shape.set_material(u32{}) };
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

template<typename ShapeT>
struct dyn_shape {
    virtual ~dyn_shape() = default;

    virtual constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection> = 0;

    virtual constexpr auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> = 0;

    virtual constexpr auto intersects(ray const& ray) const -> bool = 0;

    virtual constexpr void set_material(u32 idx) {}

    virtual constexpr void construct_tree(std::vector<ShapeT> shapes, usize depth) = 0;
    virtual constexpr auto deconstruct_tree() -> std::vector<ShapeT> = 0;

    virtual constexpr void append(std::vector<ShapeT> shapes, usize splits) {
        std::vector<ShapeT> existing_shapes = deconstruct_tree();

        existing_shapes.reserve(existing_shapes.size() + shapes.size());
        std::copy(shapes.begin(), shapes.end(), std::back_inserter(existing_shapes));

        return construct_tree(std::move(existing_shapes), splits);
    }
};

}
