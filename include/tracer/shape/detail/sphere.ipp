#pragma once

namespace trc::shapes {

constexpr auto sphere::intersect(ray const& ray) const -> std::optional<intersection> {
    real t = TRYX(intersect_impl(ray));

    if (t < 0)
        return std::nullopt;

    vec3 isection_point = ray.origin + t * ray.direction;
    vec3 normal = normal_at(isection_point);
    vec3 local_pt = isection_point - center_point;

    vec2 uv;
    vec3 dpdu;
    vec3 dpdv;

    get_surface_information(local_pt, uv, dpdu, dpdv);

    vec3 wo = -ray.direction;

    intersection ret(material_index, wo, t, isection_point, uv, {dpdu, dpdv});

    return ret;
}

template<typename Gen>
constexpr auto sphere::sample_surface(Gen& gen) const -> intersection {
    vec3 isection_point = center_point + stf::random::sphere_sampler<2>::sample<real>(gen) * radius;
    vec3 normal = normal_at(isection_point);
    vec3 local_pt = isection_point - center_point;

    vec2 uv;
    vec3 dpdu;
    vec3 dpdv;

    get_surface_information(local_pt, uv, dpdu, dpdv);

    intersection ret(material_index, vec3(), 0, isection_point, uv, {dpdu, dpdv});

    return ret;
}

constexpr auto sphere::normal_at(vec3 pt) const -> vec3 {
    return (pt - center_point) / radius;
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

constexpr void sphere::get_surface_information(vec3 local_pt, vec2& uv, vec3& dpdu, vec3& dpdv) const {
    real theta_min = std::acos(-1);
    real theta_max = std::acos(1);

    real phi = std::atan2(local_pt[1], local_pt[0]);
    if (phi < 0) phi += 2 * std::numbers::pi_v<real>;
    real theta = std::acos(std::clamp<real>(local_pt[2] / radius, -1, 1));

    real u = phi * real(0.5) * std::numbers::inv_pi_v<real>;
    real v = (theta - theta_min) / (theta_max - theta_min);

    //real u = real(0.5) - std::atan2(normal[0], normal[2]) * real(0.5) * std::numbers::inv_pi_v<real>;
    //real v = real(0.5) - std::asin(normal[1] / radius) * std::numbers::inv_pi_v<real>;

    real dx_du = -2 * std::numbers::pi_v<real> * local_pt[1];
    real dy_du = 2 * std::numbers::pi_v<real> * local_pt[0];
    real dz_du = 0;

    real z_r = std::sqrt(local_pt[0] * local_pt[0] + local_pt[1] * local_pt[1]);
    real inv_z_r = 1 / z_r;
    real cos_phi = local_pt[0] * inv_z_r;
    real sin_phi = local_pt[1] * inv_z_r;

    real dx_dv = local_pt[2] * cos_phi;
    real dy_dv = local_pt[2] * sin_phi;
    real dz_dv = -radius * std::sin(theta);

    dpdu = vec3{dx_du, dy_du, dz_du};
    dpdv = (theta_max - theta_min) * vec3{dx_dv, dy_dv, dz_dv};
    uv = vec2{u, v};
}

}// namespace trc::shapes
