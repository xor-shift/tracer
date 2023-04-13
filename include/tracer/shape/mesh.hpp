#pragma once

#include <tracer/bvh/tree.hpp>
#include <tracer/shape/shape.hpp>
#include <tracer/shape/triangle.hpp>

namespace trc::shapes {

template<std::unsigned_integral IndexType = u32>
struct pseudo_triangle {
    std::array<IndexType, 3> vertex_indices;
    vec3 normal;
};

template<std::unsigned_integral IndexType = u32>
struct mesh : generic_bvh<pseudo_triangle<IndexType>> {
    static constexpr IndexType bad_index = -1;
    using triangle_type = pseudo_triangle<IndexType>;
    using node_type = typename generic_bvh<triangle_type>::node_type;

    constexpr mesh(u32 mat_idx)
        : m_mat_idx(mat_idx) {}

    /// Adds a triangle to the mesh.\n
    /// Reduces the number of stored vertices on a best-effort basis; expect around 2/3 of the pushed vertices to be
    /// stored if you are pushing the triangles of an STL file (unlike PLY files where the mesh will contain exactly 1/3
    /// of the vertices) (this is inexact and anecdotal).\n
    /// Call finish_construction after pushing all triangles.
    constexpr void push_triangle(std::array<vec3, 3> vertices) {
        std::array<IndexType, 3> indices;
        indices[0] = find_vertex(vertices[0]);
        indices[1] = find_vertex(vertices[1]);
        indices[2] = find_vertex(vertices[2]);

        for (usize i = 0; i < 3; i++) {
            if (indices[i] == bad_index) {
                indices[i] = push_vertex(vertices[i]);
            }
        }

        push_triangle(indices);
    }

    /// Call finish_construction after pushing all triangles.
    constexpr void push_triangle(std::array<IndexType, 3> indices) {
        vec3 const& vert_0 = m_vertices[indices[0]];
        vec3 const& vert_1 = m_vertices[indices[1]];
        vec3 const& vert_2 = m_vertices[indices[2]];

        vec3 edge_0 = vert_1 - vert_0;
        vec3 edge_1 = vert_2 - vert_0;

        m_center_sum = m_center_sum + (vert_0 + vert_1 + vert_2) / 3;
        m_surface_area += abs(cross(edge_0, edge_1)) / 2;

        this->m_shapes.push_back({indices, normalize(cross(edge_0, edge_1))});
    }

    constexpr void reserve_vertices(usize amt) {
        m_vertices.reserve(m_vertices.size() + amt);
    }

    constexpr auto push_vertex(vec3 vert) -> IndexType {
        m_bounds.bump(vert);
        m_vertices.emplace_back(std::move(vert));
        return static_cast<IndexType>(m_vertices.size() - 1);
    }

    /// Do NOT Call this after finish_construction()
    constexpr void transform(mat4x4 const& mat) {
        m_bounds = {};
        for (vec3& vert : m_vertices) {
            vec4 temp(vert, 1);
            temp = mat * temp;
            vert = vec3(temp) / temp[3];
            m_bounds.bump(vert);
        }

        m_center_sum = vec3{};
        m_surface_area = 0;
        for (pseudo_triangle<IndexType>& tri : this->m_shapes) {
            vec4 temp(tri.normal, 0);
            temp = mat * temp;
            tri.normal = normalize(vec3(temp));

            vec3 const& vert_0 = m_vertices[tri.vertex_indices[0]];
            vec3 const& vert_1 = m_vertices[tri.vertex_indices[1]];
            vec3 const& vert_2 = m_vertices[tri.vertex_indices[2]];
            vec3 edge_0 = vert_1 - vert_0;
            vec3 edge_1 = vert_2 - vert_0;

            m_center_sum = m_center_sum + (vert_0 + vert_1 + vert_2) / 3;
            m_surface_area += abs(cross(edge_0, edge_1)) / 2;
        }
    }

    /// Call this function before calling intersection functions
    constexpr void finish_construction(usize depth = 12) {
        this->construct_tree(
          depth,
          [this](pseudo_triangle<IndexType> triangle) -> vec3 {// center_fn
              return (m_vertices[triangle.vertex_indices[0]] +
                      m_vertices[triangle.vertex_indices[1]] +
                      m_vertices[triangle.vertex_indices[2]]) /
                     3;
          },
          [this](pseudo_triangle<IndexType> triangle) -> std::pair<vec3, vec3> {// bounds_fn
              bounding_box bounds{};

              bounds.bump(m_vertices[triangle.vertex_indices[0]]);
              bounds.bump(m_vertices[triangle.vertex_indices[1]]);
              bounds.bump(m_vertices[triangle.vertex_indices[2]]);

              return bounds.bounds;
          });
    }

    friend constexpr void swap(mesh& lhs, mesh& rhs) {
        // you know what, i am not going to bother
        // future me, please FIXME, thanks :)

        // the issue is that the current me doesn't know how to best swap base classes
        // (without somehow befriending the mesh overload of swap() from within the base class)

        mesh temporary = std::move(lhs);
        lhs = std::move(rhs);
        rhs = std::move(temporary);
    }

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection> {
        pixel_statistics stats{};
        return intersect(ray, stats, best_t);
    }

    constexpr auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> {
        std::optional<intersection> best_isection = std::nullopt;

        auto iterate = [&best_isection, &best_t](std::optional<intersection> isection) {
            if (!isection)
                return;

            if (best_t > isection->t) {
                best_t = isection->t;
                best_isection = isection;
            }
        };

        generic_bvh<triangle_type>::traverse_candidates(ray, [&](node_type const& node) {
            iterate(node.intersect(this->m_shapes, ray, stats, best_t, [&](triangle_type const& shape, ::trc::ray const& ray, real best_t) -> std::optional<intersection> {
                std::array<vec3, 3> vertices{m_vertices[shape.vertex_indices[0]], m_vertices[shape.vertex_indices[1]], m_vertices[shape.vertex_indices[2]]};
                moller_trumbore_result res = TRYX(moller_trumbore(ray, vertices));

                vec3 global_pt = ray.origin + res.t * ray.direction;

                return intersection(m_mat_idx, -ray.direction, res.t, global_pt, res.uv, {res.edge_0, res.edge_1}, shape.normal);
            }));
        });

        return best_isection;
    }

    constexpr auto intersects(ray const& ray) const -> bool { return intersect(ray) != std::nullopt; }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return m_bounds.bounds; }

    constexpr auto center() const -> vec3 { return m_center_sum / static_cast<real>(this->m_shapes.size()); }

    template<typename Gen>
    constexpr auto sample_surface(Gen&) const -> intersection;

    constexpr auto surface_area() const -> real { return m_surface_area; }

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

    constexpr void set_material(u32 idx) { m_mat_idx = idx; }

private:
    std::vector<vec3> m_vertices;

    bounding_box m_bounds{};
    real m_surface_area = 0;
    vec3 m_center_sum = 0;

    u32 m_mat_idx;

    constexpr auto find_vertex(vec3 vert) -> IndexType {
        constexpr usize max_window_size = 3;

        usize window_size = std::min(max_window_size, m_vertices.size());

        for (usize wi = 0; wi < window_size; wi++) {
            usize i = m_vertices.size() - wi - 1;

            if (m_vertices[i] == vert) {
                return static_cast<IndexType>(i);
            }
        }

        return bad_index;
    }
};

}// namespace trc::shapes
