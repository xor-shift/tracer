#pragma once

#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>
#include <tracer/shape/shape.hpp>

#include <optional>

namespace trc::shapes {

struct plane {
    constexpr plane() = default;

    constexpr plane(u32 mat_idx, vec3 center, vec3 normal) noexcept
        : m_center(center)
        , m_normal(normal)
        , m_mat_idx(mat_idx) {}

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection>;
    
    constexpr auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> {
        ++stats.shape_intersection_tests;
        return intersect(ray, best_t);
    }

    constexpr auto intersects(ray const& ray) const -> bool;

    constexpr auto normal_at(vec3 pt) const -> vec3 { return m_normal; }

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

    constexpr void set_material(u32 idx) { m_mat_idx = idx; }

    friend constexpr void swap(plane&, plane&);

private:
    vec3 m_center;
    vec3 m_normal;
    u32 m_mat_idx;
};

constexpr void swap(plane& lhs, plane& rhs) {
    using std::swap;

    swap(lhs.m_center, rhs.m_center);
    swap(lhs.m_normal, rhs.m_normal);
    swap(lhs.m_mat_idx, rhs.m_mat_idx);
}

static_assert(concepts::unbound_shape<plane>);

}// namespace trc::shapes

#include <tracer/shape/detail/plane.ipp>
