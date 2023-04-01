#pragma once

namespace trc::shapes {

constexpr auto disc::intersect(ray const& ray) const -> std::optional<intersection> {
    // ray-plane intersection
    real t = dot(m_center - ray.origin, m_normal) / dot(m_normal, ray.direction);

    if (t <= 0 || std::isinf(t)) {
        return std::nullopt;
    }

    vec3 isection_point = ray.origin + t * ray.direction;
    vec3 p = isection_point - m_center;
    real r_p = abs(p);

    if (r_p >= m_radius) {
        return std::nullopt;
    }

    vec2 uv;
    vec3 dpdu;
    vec3 dpdv;

    get_surface_information(p, uv, dpdu, dpdv);

    intersection ret(m_mat_idx, -ray.direction, t, isection_point, uv, {dpdu, dpdv});

    return ret;
}

constexpr auto disc::intersects(ray const& ray) const -> bool { return !!intersect(ray); }

template<typename Gen>
constexpr auto disc::sample_surface(Gen& gen) const -> intersection {
    vec2 uv = stf::random::ball_sampler<2>::sample<real>(gen);

    auto [ortho_u, ortho_v] = detail::get_dummy_dp_duv(m_normal);

    auto isection_point = m_center + ortho_u * uv[0] + ortho_v * uv[1];

    intersection ret(m_mat_idx, vec3(), 0, isection_point, uv, {ortho_u, ortho_v});

    return ret;
}

constexpr void disc::get_surface_information(vec3 p, vec2& uv, vec3& dpdu, vec3& dpdv) const {
    auto [ortho_u, ortho_v] = detail::get_dummy_dp_duv(m_normal);

    dpdu = ortho_u;
    dpdv = ortho_v;
    uv = vec2(dot(ortho_u, p), dot(ortho_v, p)) / m_radius * 0.5 + vec2(0.5);

    // we imagine the disc to be centered around the origin
    //
    // p  -> intersection point
    // rₚ -> p's distance from the disc's center (also the v coordinate after being scaled)
    // θ  -> the angle the line that intersects the XZ plane makes with the X axis (default is 0 rad)
    // φ  -> the angle the disc makes with the XZ plane
    // β  -> the angle r_p makes with the line defining θ (also the u coordinate after being scaled)
    //
    //TODO: continue this for a better UV representation
}

}// namespace trc::shapes
