#pragma once

#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <optional>

namespace trc::shapes {

struct plane {
    constexpr plane() = default;

    constexpr plane(u32 mat_idx, vec3 center, vec3 normal) noexcept
        : m_center(center)
        , m_normal(normal)
        , m_mat_idx(mat_idx) {}

    constexpr auto intersect(ray const& ray) const -> std::optional<intersection>;

    constexpr auto intersects(ray const& ray) const -> bool;

    constexpr auto normal_at(vec3 pt) const -> vec3 { return m_normal; }

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

private:
    vec3 m_center;
    vec3 m_normal;
    u32 m_mat_idx;
};

static_assert(concepts::unbound_shape<plane>);

}// namespace trc::shapes

#include <tracer/shape/detail/plane.ipp>
