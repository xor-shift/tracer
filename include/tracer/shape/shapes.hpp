#pragma once

#include <tracer/shape/concepts.hpp>

#include <tracer/shape/disc.hpp>
#include <tracer/shape/sphere.hpp>
#include <tracer/shape/plane.hpp>

#include <variant>

namespace trc {

using bound_shape = std::variant<shapes::sphere, shapes::disc>;
using unbound_shape = std::variant<shapes::plane>;

using shape = std::variant<shapes::sphere, shapes::disc, shapes::plane>;

template<concepts::shape... Ts>
constexpr auto intersect(std::variant<Ts...> const& shape, ray const& ray) -> std::optional<intersection> {
    return std::visit([&ray](concepts::shape auto const& shape) {
        return shape.intersect(ray);
    }, shape);
}

template<concepts::shape... Ts>
constexpr auto material_index(std::variant<Ts...> const& shape) -> u32 {
    return std::visit([](concepts::shape auto const& shape) {
        return shape.material_index;
    }, shape);
}

}
