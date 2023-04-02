#pragma once

namespace trc::shapes {

constexpr triangle::triangle(u32 mat_idx, std::array<vec3, 3> vertices) noexcept
    : triangle(mat_idx, vertices, {{{0, 0}, {1, 0}, {0, 1}}}) {
}

constexpr triangle::triangle(u32 mat_idx, std::array<vec3, 3> vertices, std::array<vec2, 3> vertex_uvs) noexcept
    : m_vertices(vertices)
    , m_edges({vertices[1] - vertices[0], vertices[2] - vertices[0]})
    , m_vertex_uvs(vertex_uvs)
    , m_center(compute_center(vertices, center_type::incenter))
    , m_mat_idx(mat_idx) {
    real inf = std::numeric_limits<real>::infinity();

    vec3 min_extent{inf, inf, inf};
    vec3 max_extent{-inf, -inf, -inf};

    for (auto vertex: vertices) {
        min_extent = min(min_extent, vertex);
        max_extent = max(max_extent, vertex);
    }

    m_extents.first = min_extent;
    m_extents.second = max_extent;

    m_surface_area = abs(cross(m_edges[0], m_edges[1])) / 2;
}

constexpr auto triangle::intersect(ray const& ray, real best_t) const -> std::optional<intersection> {
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

    vec3 global_pt = ray.origin + t * ray.direction;

    return intersection(m_mat_idx, -ray.direction, t, global_pt, {u, v}, {m_edges[0], m_edges[1]});
}

constexpr auto triangle::compute_center(std::array<vec3, 3> const& vertices, center_type type) -> vec3 {
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

}// namespace trc::shapes
