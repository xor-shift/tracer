#pragma once

#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>
#include <tracer/shape/shape.hpp>

#include <stuff/ranvec.hpp>

#include <optional>

namespace trc::shapes {

struct moller_trumbore_result {
    vec3 edge_0;
    vec3 edge_1;
    vec2 uv;
    real t;
};

constexpr auto moller_trumbore(ray const& ray, std::span<const vec3, 3> vertices) -> std::optional<moller_trumbore_result> {
    vec3 edge_0 = vertices[1] - vertices[0];
    vec3 edge_1 = vertices[2] - vertices[0];

    vec3 h = cross(ray.direction, edge_1);
    real a = dot(edge_0, h);

    if (std::abs(a) <= epsilon)
        return std::nullopt;

    real f = 1 / a;

    vec3 s = ray.origin - vertices[0];
    real u = f * dot(s, h);
    if (u < 0 || u > 1)
        return std::nullopt;

    vec3 q = cross(s, edge_0);
    real v = f * dot(ray.direction, q);
    if (v < 0 || u + v > 1)
        return std::nullopt;

    real t = f * dot(edge_1, q);
    if (t <= epsilon)
        return std::nullopt;

    return moller_trumbore_result{
      .edge_0 = edge_0,
      .edge_1 = edge_1,
      .uv = vec2{u, v},
      .t = t,
    };
}

struct triangle {
    constexpr triangle(u32 mat_idx, std::array<vec3, 3> vertices) noexcept;

    constexpr triangle(u32 mat_idx, std::array<vec3, 3> vertices, std::array<vec2, 3> vertex_uvs) noexcept;

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection>;

    constexpr auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> {
        ++stats.shape_intersection_tests;
        return intersect(ray, best_t);
    }

    constexpr auto intersects(ray const& ray) const -> bool { return intersect(ray) != std::nullopt; }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return m_extents; }

    constexpr auto center() const -> vec3 { return m_center; }

    template<typename Gen>
    constexpr auto sample_surface(Gen&) const -> intersection;

    constexpr auto surface_area() const -> real { return abs(cross(m_vertices[1] - m_vertices[0], m_vertices[2] - m_vertices[0])) / 2; }

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

    constexpr void set_material(u32 idx) { m_mat_idx = idx; }

    friend constexpr void swap(triangle&, triangle&);

private:
    std::array<vec3, 3> m_vertices;
    std::array<vec2, 3> m_vertex_uvs;

    std::pair<vec3, vec3> m_extents;
    vec3 m_center;

    u32 m_mat_idx;

    enum class center_type {
        incenter,
        centroid,
        circumcenter,
        orthocenter,
    };

    static constexpr auto compute_center(std::array<vec3, 3> const& vertices, center_type type = center_type::incenter) -> vec3;
};

constexpr void swap(triangle& lhs, triangle& rhs) {
    using std::swap;

    swap(lhs.m_vertices, rhs.m_vertices);
    swap(lhs.m_vertex_uvs, rhs.m_vertex_uvs);
    swap(lhs.m_extents, rhs.m_extents);
    swap(lhs.m_center, rhs.m_center);
    swap(lhs.m_mat_idx, rhs.m_mat_idx);
}

static_assert(concepts::bound_shape<triangle>);

}// namespace trc::shapes

#include <tracer/shape/detail/triangle.ipp>
