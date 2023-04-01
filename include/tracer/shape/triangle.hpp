#pragma once

#include <stuff/ranvec.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <optional>

namespace trc::shapes {

struct triangle {
    constexpr triangle(u32 mat_idx, std::array<vec3, 3> vertices, std::array<vec2, 3> vertex_uvs) noexcept
        : m_vertices(vertices)
        , m_edges({vertices[1] - vertices[0], vertices[2] - vertices[0]})
        , m_vertex_uvs(vertex_uvs)
        , m_center(compute_center(vertices, center_type::incenter))
        , m_mat_idx(mat_idx) {
        real inf = std::numeric_limits<real>::infinity();

        vec3 min_extent{inf, inf, inf};
        vec3 max_extent{-inf, -inf, -inf};

        for (auto vertex: vertices) {
            min_extent[0] = std::min(min_extent[0], vertex[0]);
            min_extent[1] = std::min(min_extent[1], vertex[1]);
            min_extent[2] = std::min(min_extent[2], vertex[2]);

            max_extent[0] = std::max(max_extent[0], vertex[0]);
            max_extent[1] = std::max(max_extent[1], vertex[1]);
            max_extent[2] = std::max(max_extent[2], vertex[2]);
        }

        m_extents.first = min_extent;
        m_extents.second = max_extent;

        m_surface_area = abs(cross(m_edges[0], m_edges[1])) / 2;
    }

    constexpr auto intersect(ray const& ray) const -> std::optional<intersection> {
        vec3 h = cross(ray.direction, m_edges[1]);
        real a = dot(m_edges[0], h);

        if (std::abs(a) <= epsilon)
            return std::nullopt;

        real f = 1 / a;

        vec3 s = ray.origin - m_vertices[0];
        real u = f * dot(s, h);
        if (u < 0 || u > 1)
            return std::nullopt;

        vec3 q = cross(s, m_edges[0]);
        real v = f * dot(ray.direction, q);
        if (v < 0 || u + v > 1)
            return std::nullopt;

        real t = f * dot(m_edges[1], q);
        if (t <= epsilon)
            return std::nullopt;

        return std::nullopt;
    }

    constexpr auto intersects(ray const& ray) const -> bool { return intersect(ray) != std::nullopt; }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return m_extents; }

    constexpr auto center() const -> vec3 { return m_center; }

    template<typename Gen>
    constexpr auto sample_surface(Gen&) const -> vec3;

    constexpr auto normal_at(vec3 pt) const -> vec3 { return m_normal; }

    constexpr auto surface_area() const -> real { return m_surface_area; }

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

private:
    std::array<vec3, 3> m_vertices;
    std::array<vec3, 2> m_edges;
    std::array<vec2, 3> m_vertex_uvs;

    std::pair<vec3, vec3> m_extents;
    vec3 m_center;
    vec3 m_normal;
    real m_surface_area;

    uint32_t m_mat_idx;

    enum class center_type {
        incenter,
        centroid,
        circumcenter,
        orthocenter,
    };

    static constexpr auto compute_center(std::array<vec3, 3> const& vertices, center_type type = center_type::incenter) -> vec3 {
        real a = abs(vertices[1] - vertices[2]);
        real b = abs(vertices[2] - vertices[0]);
        real c = abs(vertices[0] - vertices[1]);

        std::array<real, 3> w{};

        switch (type) {
            case center_type::incenter:
                return (vertices[0] + vertices[1] + vertices[2]) / 3;
            case center_type::centroid:
                std::unreachable();
            case center_type::circumcenter:
                std::unreachable();
            case center_type::orthocenter:
                std::unreachable();
        }

        return (vertices[0] * w[0] + vertices[1] * w[1] + vertices[2] * w[2]) / (w[0] + w[1] + w[2]);
    }
};

}// namespace trc::shapes
