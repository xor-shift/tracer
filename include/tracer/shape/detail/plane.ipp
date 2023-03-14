#pragma once

namespace trc::shapes {

constexpr auto plane::intersect(ray const& ray) const -> std::optional<intersection> {
    real divisor = dot(normal, ray.direction);
    real t = dot(center - ray.origin, normal) / divisor;

    if (t <= 0 || std::isinf(t))
        return std::nullopt;

    vec3 isection_point = ray.origin + t * ray.direction;

    vec3 ortho_u = detail::get_orthogonal_vector(normal);
    vec3 ortho_v = cross(normal, ortho_u);
    vec2 uv(dot(ortho_u, isection_point), dot(ortho_v, isection_point));

    intersection intersection{
      .isection_point = isection_point,
      .uv = uv,

      .wo = -ray.direction,
      .t = t,

      .normal = normal,
      .material_index = material_index,
    };

    return intersection;
}

constexpr auto plane::intersects(ray const& ray) const -> bool {
    real divisor = dot(normal, ray.direction);
    real t = dot(center - ray.origin, normal) / divisor;

    return !(t <= 0 || std::isinf(t));
}

}// namespace trc::shapes
