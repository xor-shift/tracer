#pragma once

#include <stuff/ranvec.hpp>

#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>
#include <tracer/shape/shape.hpp>

#include <optional>

namespace trc {

constexpr auto check_bounds_intersection(ray const& ray, std::pair<vec3, vec3> const& extents) -> bool;

struct bounding_box {
    std::pair<vec3, vec3> bounds {vec3(std::numeric_limits<real>::infinity()), vec3(-std::numeric_limits<real>::infinity())};

    constexpr void bump(vec3 vec) {
        bounds.first = stf::blas::min(bounds.first, vec);
        bounds.second = stf::blas::max(bounds.second, vec);
    }

    constexpr void bump(bounding_box const& other) {
        bump(other.bounds.first);
        bump(other.bounds.second);
    }

    constexpr void bump(std::pair<vec3, vec3> const& bounds) {
        bump(bounds.first);
        bump(bounds.second);
    }

    constexpr auto intersects(ray const& ray) const -> bool {
        return check_bounds_intersection(ray, bounds);
    }

    constexpr void extend_a_little() {
        bounds.first = bounds.first * (1 + epsilon);
        bounds.second = bounds.second * (1 + epsilon);
    }
};

}// namespace trc


namespace trc::shapes {

struct box {
    constexpr box(u32 mat_idx, std::pair<vec3, vec3> corners)
        : m_mat_idx(mat_idx) {
        for (usize i = 0; i < 3; i++) {
            m_extents.first[i] = std::min(corners.first[i], corners.second[i]);
            m_extents.second[i] = std::max(corners.first[i], corners.second[i]);
        }
    }

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection>;

    constexpr auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> {
        ++stats.shape_intersection_tests;
        return intersect(ray, best_t);
    }

    constexpr auto intersects(ray const& ray) const -> bool {
        return check_bounds_intersection(ray, m_extents);
    }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return m_extents; }

    constexpr auto center() const -> vec3 { return (m_extents.first + m_extents.second) / 2; }

    template<typename Gen>
    constexpr auto sample_surface(Gen& gen) const -> intersection;

    constexpr auto normal_at(vec3 pt) const -> vec3;

    constexpr auto uv_at(vec3 pt) const -> vec2;

    constexpr auto surface_area() const -> real;

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

    constexpr void set_material(u32 idx) { m_mat_idx = idx; }

    friend constexpr void swap(box&, box&);

private:
    std::pair<vec3, vec3> m_extents;

    u32 m_mat_idx;

    constexpr auto side_lengths() const -> vec3;

    static constexpr auto impl(ray const& ray, std::pair<vec3, vec3> const& extents) -> std::pair<real, real>;

    friend constexpr auto ::trc::check_bounds_intersection(ray const& ray, std::pair<vec3, vec3> const& extents) -> bool;
};

constexpr void swap(box& lhs, box& rhs) {
    using std::swap;

    swap(lhs.m_extents, rhs.m_extents);
    swap(lhs.m_mat_idx, rhs.m_mat_idx);
}

static_assert(concepts::bound_shape<box>);

}// namespace trc::shapes

#include <tracer/shape/detail/box.ipp>
