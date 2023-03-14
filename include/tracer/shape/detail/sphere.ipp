#pragma once

namespace trc::shapes {

constexpr auto sphere::intersect(ray const& ray) const -> std::optional<intersection> {
    real t = TRYX(intersect_impl(ray));

    if (t < 0)
        return std::nullopt;

    vec3 isection_point = ray.origin + t * ray.direction;
    vec3 normal = normal_at(isection_point);

    real u = real(0.5) - std::atan2(normal[0], normal[2]) * real(0.5) * std::numbers::inv_pi_v<real>;
    real v = real(0.5) - std::asin(normal[1] / radius) * std::numbers::inv_pi_v<real>;

    intersection ret{
      .isection_point = isection_point,
      .uv = vec2(u, v),

      .wo = -ray.direction,
      .t = t,

      .normal = normal,
      .material_index = material_index,
    };

    return ret;
}

template<typename Gen>
constexpr auto sphere::sample_surface(Gen& gen) const -> intersection {
    vec3 isection_point = center_point + stf::random::sphere_sampler<2>::sample(gen) * radius;
    vec3 normal = normal_at(isection_point);

    real u = real(0.5) - std::atan2(normal[0], normal[2]) * real(0.5) * std::numbers::inv_pi_v<real>;
    real v = real(0.5) - std::asin(normal[1] / radius) * std::numbers::inv_pi_v<real>;

    intersection ret{
      .isection_point = isection_point,
      .uv = vec2(u, v),

      .wo = vec3(0),
      //.t = 0,

      .normal = normal,
      .material_index = material_index,
    };

    return ret;
}

constexpr auto sphere::normal_at(vec3 pt) const -> vec3 {
    return pt - center_point;
}

constexpr auto sphere::intersect_impl(ray const& ray) const -> std::optional<real> {
    vec3 dir = ray.origin - center_point;

    real a = dot(ray.direction, ray.direction);
    real b = 2 * dot(dir, ray.direction);
    real c = dot(dir, dir) - radius * radius;

    real discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        return std::nullopt;

    real t_0 = (-b + std::sqrt(discriminant)) / (2 * a);
    real t_1 = (-b - std::sqrt(discriminant)) / (2 * a);

    real t = t_0 < 0 ? t_1 : std::min(t_0, t_1);

    return t;
}

}// namespace trc::shapes
