#pragma once

#include <stuff/ranvec.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <optional>

namespace trc::shapes {

struct triangle {
    std::array<vec3, 3> vertices;
    std::array<vec3, 2> uv_vertices;

    constexpr auto intersect(ray const&) const -> std::optional<intersection> { return std::nullopt; }

    constexpr auto intersects(ray const&) const -> bool { return false; }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return {center_point, center_point}; }

    constexpr auto center() const -> vec3 { return center_point; }

    template<typename Gen>
    constexpr auto sample_surface(Gen&) const -> vec3 { return center_point; }
};

}// namespace trc::shapes
