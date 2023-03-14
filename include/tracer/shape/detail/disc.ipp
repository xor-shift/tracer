#pragma once

namespace trc::shapes {

constexpr auto disc::intersect(ray const& ray) const -> std::optional<intersection> {
    real divisor = dot(normal, ray.direction);
    real t = dot(center_point - ray.origin, normal) / divisor;

    if (t <= 0 || std::isinf(t)) {
        return std::nullopt;
    }

    vec3 isection_point = ray.origin + t * ray.direction;
    vec3 offset = isection_point - center_point;
    real dist_from_center = abs(offset);

    if (dist_from_center >= radius) {
        return std::nullopt;
    }

    auto [ortho_u, ortho_v] = get_surface_basis();

    vec2 uv = vec2(dot(ortho_u, offset), dot(ortho_v, offset)) / radius * 0.5 + vec2(0.5);

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

constexpr auto disc::intersects(ray const& ray) const -> bool { return !!intersect(ray); }

template<typename Gen>
constexpr auto disc::sample_surface(Gen& gen) const -> intersection {
    vec2 uv = stf::random::ball_sampler<2>::sample<real>(gen);

    auto [ortho_u, ortho_v] = get_surface_basis();

    auto isection_point = center_point + ortho_u * uv[0] + ortho_v * uv[1];

    intersection intersection{
      .isection_point = isection_point,
      .uv = uv,

      .wo = vec3(0),
      .t = 0,

      .normal = normal,
      .material_index = material_index,
    };

    return intersection;
}

constexpr auto disc::get_surface_basis() const -> std::pair<vec3, vec3> {
    vec3 ortho_u = detail::get_orthogonal_vector(normal);
    vec3 ortho_v = cross(normal, ortho_u);
    return {ortho_u, ortho_v};
}

}// namespace trc::shapes
