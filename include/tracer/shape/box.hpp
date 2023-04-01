#pragma once

#include <stuff/ranvec.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <optional>

namespace trc {

constexpr auto check_bounds_intersection(ray const& ray, std::pair<vec3, vec3> const& extents) -> bool;

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

    constexpr auto intersect(ray const& ray) const -> std::optional<intersection>;

    constexpr auto intersects(ray const& ray) const -> bool {
        return check_bounds_intersection(ray, m_extents);
    }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return m_extents; }

    constexpr auto center() const -> vec3 { return (m_extents.first + m_extents.second) / 2; }

    template<typename Gen>
    constexpr auto sample_surface(Gen& gen) const -> intersection;

    constexpr auto normal_at(vec3 pt) const -> vec3;

    constexpr auto surface_area() const -> real;

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

private:
    std::pair<vec3, vec3> m_extents;

    u32 m_mat_idx;

    constexpr auto side_lengths() const -> vec3;

    static constexpr auto impl(ray const& ray, std::pair<vec3, vec3> const& extents) -> std::pair<real, real>;

    friend constexpr auto ::trc::check_bounds_intersection(ray const& ray, std::pair<vec3, vec3> const& extents) -> bool;
};

static_assert(concepts::bound_shape<box>);

}// namespace trc::shapes

#include <tracer/shape/detail/box.ipp>
