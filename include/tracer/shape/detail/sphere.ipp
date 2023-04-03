#pragma once

namespace trc::shapes {

constexpr auto sphere::intersect(ray const& ray, real best_t) const -> std::optional<intersection> {
    real t = TRYX(intersect_impl(ray));

    if (t < 0 || t > best_t)
        return std::nullopt;

    vec3 isection_point = ray.origin + t * ray.direction;
    vec3 local_pt = isection_point - m_center;

    vec2 uv;
    vec3 dpdu;
    vec3 dpdv;

    get_surface_information(local_pt, uv, dpdu, dpdv);

    vec3 wo = -ray.direction;

    if (dot(wo, local_pt / m_radius) < 0) {
        std::ignore = 0;
    }

    intersection ret(m_mat_idx, wo, t, isection_point, uv, {dpdu, dpdv});

    return ret;
}

template<typename Gen>
constexpr auto sphere::sample_surface(Gen& gen) const -> intersection {
    vec3 isection_point = m_center + stf::random::sphere_sampler<2>::sample<real>(gen) * m_radius;
    vec3 local_pt = isection_point - m_center;

    vec2 uv;
    vec3 dpdu;
    vec3 dpdv;

    get_surface_information(local_pt, uv, dpdu, dpdv);

    intersection ret(m_mat_idx, vec3(), 0, isection_point, uv, {dpdu, dpdv});

    return ret;
}

constexpr auto sphere::normal_at(vec3 global_pt) const -> vec3 {
    return (global_pt - m_center) / m_radius;
}

constexpr auto sphere::intersect_impl(ray const& ray) const -> std::optional<real> {
    vec3 dir = ray.origin - m_center;

    real a = dot(ray.direction, ray.direction);
    real b = 2 * dot(dir, ray.direction);
    real c = dot(dir, dir) - m_radius * m_radius;

    real discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        return std::nullopt;

    real t_0 = (-b + std::sqrt(discriminant)) / (2 * a);
    real t_1 = (-b - std::sqrt(discriminant)) / (2 * a);

    real t = t_0 < 0 ? t_1 : std::min(t_0, t_1);

    return t;
}

constexpr void sphere::get_surface_information(vec3 local_pt, vec2& uv, vec3& dpdu, vec3& dpdv) const {
    real phi;
    real theta;
    auto spherical_coordinates = std::tie(phi, theta);
    std::tie(uv, spherical_coordinates) = detail::get_sphere_uv(local_pt, m_radius);

    real dx_du = -2 * std::numbers::pi_v<real> * local_pt[1];
    real dy_du = 2 * std::numbers::pi_v<real> * local_pt[0];
    real dz_du = 0;

    real z_r = std::sqrt(local_pt[0] * local_pt[0] + local_pt[1] * local_pt[1]);
    real inv_z_r = 1 / z_r;
    real cos_phi = local_pt[0] * inv_z_r;
    real sin_phi = local_pt[1] * inv_z_r;

    real dx_dv = local_pt[2] * cos_phi;
    real dy_dv = local_pt[2] * sin_phi;
    real dz_dv = -m_radius * std::sin(theta);

    dpdu = vec3{dx_du, dy_du, dz_du};
    dpdv = -std::numbers::pi_v<real> * vec3{dx_dv, dy_dv, dz_dv};
}

}// namespace trc::shapes
