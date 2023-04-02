#pragma once

#include <tracer/shape/detail/util.hpp>

namespace trc::shapes {

constexpr auto plane::intersect(ray const& ray, real best_t) const -> std::optional<intersection> {
    real divisor = dot(m_normal, ray.direction);
    real t = dot(m_center - ray.origin, m_normal) / divisor;

    if (t < 0 || t > best_t || std::isinf(t))
        return std::nullopt;

    vec3 isection_point = ray.origin + t * ray.direction;

    auto const [du_dp, dv_dp] = detail::get_dummy_dp_duv(m_normal);
    vec2 uv(dot(du_dp, isection_point), dot(dv_dp, isection_point));

    vec3 wo = -ray.direction;

    intersection ret(m_mat_idx, wo, t, isection_point, uv, {du_dp, dv_dp});

    return ret;
}

constexpr auto plane::intersects(ray const& ray) const -> bool {
    real divisor = dot(m_normal, ray.direction);
    real t = dot(m_center - ray.origin, m_normal) / divisor;

    return !(t <= 0 || std::isinf(t));
}

}// namespace trc::shapes
