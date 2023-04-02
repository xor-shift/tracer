#pragma once

#include <stuff/ranvec.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>
#include <tracer/shape/plane.hpp>

#include <optional>

namespace trc::shapes {

struct disc {
    constexpr disc() = default;

    constexpr disc(u32 mat_idx, vec3 center, vec3 normal, real radius) noexcept
        : m_center(center)
        , m_normal(normal)
        , m_radius(radius)
        , m_mat_idx(mat_idx) {}

    constexpr auto intersect(ray const& ray, real best_t = std::numeric_limits<real>::infinity()) const -> std::optional<intersection>;

    constexpr auto intersects(ray const& ray) const -> bool;

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return {m_center - vec3(m_radius), m_center + vec3(m_radius)}; }

    constexpr auto center() const -> vec3 { return m_center; }

    template<typename Gen>
    constexpr auto sample_surface(Gen& gen) const -> intersection;

    constexpr auto normal_at(vec3 pt) const -> vec3 { return m_normal; }

    constexpr auto surface_area() const -> real { return std::numbers::pi_v<real> * m_radius * m_radius; }

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

private:
    constexpr void get_surface_information(vec3 r_p, vec2& uv, vec3& dpdu, vec3& dpdv) const;

    vec3 m_center;
    vec3 m_normal;
    real m_radius;
    u32 m_mat_idx;
};

static_assert(concepts::bound_shape<disc>);

}// namespace trc::shapes

#include <tracer/shape/detail/disc.ipp>
