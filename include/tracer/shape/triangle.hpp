#pragma once

#include <stuff/ranvec.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <optional>

namespace trc::shapes {

struct triangle {
    constexpr triangle(u32 mat_idx, std::array<vec3, 3> vertices) noexcept;

    constexpr triangle(u32 mat_idx, std::array<vec3, 3> vertices, std::array<vec2, 3> vertex_uvs) noexcept;

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection>;

    constexpr auto intersects(ray const& ray) const -> bool { return intersect(ray) != std::nullopt; }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return m_extents; }

    constexpr auto center() const -> vec3 { return m_center; }

    template<typename Gen>
    constexpr auto sample_surface(Gen&) const -> intersection;

    constexpr auto surface_area() const -> real { return m_surface_area; }

private:
    std::array<vec3, 3> m_vertices;
    std::array<vec3, 2> m_edges;
    std::array<vec2, 3> m_vertex_uvs;

    std::pair<vec3, vec3> m_extents;
    vec3 m_center;
    real m_surface_area;

    uint32_t m_mat_idx;

    enum class center_type {
        incenter,
        centroid,
        circumcenter,
        orthocenter,
    };

    static constexpr auto compute_center(std::array<vec3, 3> const& vertices, center_type type = center_type::incenter) -> vec3;
};

static_assert(concepts::bound_shape<triangle>);

}// namespace trc::shapes

#include <tracer/shape/detail/triangle.ipp>
