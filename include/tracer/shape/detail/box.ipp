#pragma once

#include <tracer/shape/detail/util.hpp>

namespace trc {

constexpr auto check_bounds_intersection(ray const& ray, std::pair<vec3, vec3> const& extents) -> bool {
    auto const& [t_min, t_max] = shapes::box::impl(ray, extents);
    return t_max > t_min;
}

namespace shapes {

constexpr auto box::intersect(ray const& ray, real best_t) const -> std::optional<intersection> {
    auto const& [t_min, t_max] = impl(ray, m_extents);
    if (t_min >= t_max)
        return std::nullopt;

    real t = t_min > 0 ? t_min : t_max;

    if (t < 0 || t > best_t)
        return std::nullopt;

    vec3 global_pt = ray.origin + ray.direction * t;

    vec2 uv = uv_at(global_pt);
    vec3 normal = normal_at(global_pt);

    std::pair<vec3, vec3> dp_duv = detail::get_dummy_dp_duv(normal);

    return intersection(m_mat_idx, -ray.direction, t, global_pt, uv, dp_duv);
}

template<typename Gen>
constexpr auto box::sample_surface(Gen& gen) const -> intersection {
    // TODO: this function is trash
    // translation: this function is really quite inefficient on top of not being uniform for stretched boxes.
    // this also feels like a very weird of sampling a box...

    /// Shirley's low-distortion mapping from a square to a disc in the other direction.
    /// \param p A sample from the 2-unit square centered around the origin.
    constexpr auto uv_to_xy = [](vec2 uv) constexpr->vec2 {
        real u = uv[0];
        real v = uv[1];
        real u2 = u * u;
        real v2 = v * v;

        real k = std::sqrt(u2 + v2);

        auto sgn = [](real v) constexpr->real {
            return v > 0 ? 1 : -1;
        };

        real x = u2 > v2 ? sgn(u) : 4 * std::numbers::inv_pi_v<real> * std::atan(u / std::abs(v) + epsilon);
        real y = u2 > v2 ? 4 * std::numbers::inv_pi_v<real> * std::atan(v / std::abs(u)) : sgn(v);

        return {k * x, k * y};
    };

    stf::random::erand48_distribution<real> dist{};

    real angle = 2 * std::numbers::pi_v<real> * dist(gen);
    vec3 uniform_sample(uv_to_xy({std::cos(angle), std::sin(angle)}), dist(gen) * 2 - 1);

    int axis = static_cast<int>(std::trunc(dist(gen) * 3));
    std::swap(uniform_sample[axis], uniform_sample[(axis + 1) % 3]);

    vec3 global_p = uniform_sample * (side_lengths() / 2);

    vec3 normal = normal_at(global_p);

    std::pair<vec3, vec3> dp_duv = detail::get_dummy_dp_duv(normal);

    vec2 uv = uv_at(global_p);

    return intersection(m_mat_idx, vec3{}, 0, global_p, uv, dp_duv);
}

constexpr auto box::normal_at(vec3 global_pt) const -> vec3 {
    vec3 c = global_pt - center();
    vec3 d = (m_extents.first - m_extents.second) / 2;

    vec3 t = c * (epsilon + 1) / elem_abs(d);

    vec3 res = vec3{std::trunc(t[0]), std::trunc(t[1]), std::trunc(t[2])};
    res = normalize(res);
    assert_normal(res);

    return res;
}

constexpr auto box::uv_at(vec3 global_pt) const -> vec2 {
    vec3 local_p = (global_pt - center()) * 2;
    vec3 unit_p = local_p / (side_lengths() / 2);
    vec3 local_p_dir = normalize(unit_p);

    vec2 uv;
    std::tie(uv, std::ignore) = detail::get_sphere_uv(local_p_dir, 1, {std::acos(-1), std::acos(1)});

    return uv;
}

constexpr auto box::surface_area() const -> real {
    vec3 t = side_lengths();

    return t[0] * t[1] * 2 + t[1] * t[2] * 2 + t[2] * t[0] * 2;
}

constexpr auto box::side_lengths() const -> vec3 {
    vec3 t = m_extents.second - m_extents.first;
    t[0] = std::abs(t[0]);
    t[1] = std::abs(t[1]);
    t[2] = std::abs(t[2]);
    return t;
}

constexpr auto box::impl(ray const& ray, std::pair<vec3, vec3> const& extents) -> std::pair<real, real> {
    // https://tavianator.com/2022/ray_box_boundary.html

    real t_min = -std::numeric_limits<real>::infinity();
    real t_max = std::numeric_limits<real>::infinity();

    for (usize i = 0; i < 3; i++) {
        real t_1 = (extents.first[i] - ray.origin[i]) * ray.direction_reciprocals[i];
        real t_2 = (extents.second[i] - ray.origin[i]) * ray.direction_reciprocals[i];

        t_min = std::min(std::max(t_1, t_min), std::max(t_2, t_min));
        t_max = std::max(std::min(t_1, t_max), std::min(t_2, t_max));
    }

    return {t_min, t_max};
}

}// namespace shapes

}// namespace trc
