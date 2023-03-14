#pragma once

#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <optional>

namespace trc::shapes {

namespace detail {

constexpr auto get_orthogonal_vector(vec3 normal) -> vec3 {
    // slightly modified from:
    // https://computergraphics.stackexchange.com/questions/8382/how-do-i-convert-a-hit-on-an-infinite-plane-to-uv-coordinates-for-texturing-in-a

    vec3 a = cross(normal, vec3(1, 0, 0));
    vec3 b = cross(normal, vec3(0, 1, 0));
    vec3 c = cross(normal, vec3(0, 0, 1));

    vec3 max_ab = dot(a, a) <= dot(b, b) ? b : a;
    vec3 max_abc = dot(max_ab, max_ab) <= dot(c, c) ? c : max_ab;

    return normalize(max_abc);
}

}// namespace detail

struct plane {
    vec3 center;
    vec3 normal;
    u32 material_index;

    constexpr auto intersect(ray const& ray) const -> std::optional<intersection>;

    constexpr auto intersects(ray const& ray) const -> bool;

    constexpr auto normal_at(vec3 pt) const -> vec3 { return normal; }
};

static_assert(concepts::unbound_shape<plane>);

}// namespace trc::shapes

#include <tracer/shape/detail/plane.ipp>
