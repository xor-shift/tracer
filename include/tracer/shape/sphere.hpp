#pragma once

#include <stuff/expected.hpp>
#include <stuff/ranvec.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <optional>

namespace trc::shapes {

struct sphere {
    constexpr sphere() = default;

    constexpr sphere(u32 mat_idx, vec3 center, real radius) noexcept
        : m_center(center)
        , m_radius(radius)
        , m_mat_idx(mat_idx) {}

    constexpr auto intersect(ray const& ray, real best_t = std::numeric_limits<real>::infinity()) const -> std::optional<intersection>;

    constexpr auto intersects(ray const& ray) const -> bool { return !!intersect_impl(ray); }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return {m_center - vec3(m_radius), m_center + vec3(m_radius)}; }

    constexpr auto center() const -> vec3 { return m_center; }

    template<typename Gen>
    constexpr auto sample_surface(Gen& gen) const -> intersection;

    constexpr auto surface_area() const -> real {
        return 4 * std::numbers::pi_v<real> * m_radius;
    }

private:
    vec3 m_center;
    real m_radius;
    u32 m_mat_idx;

    constexpr auto intersect_impl(ray const& ray) const -> std::optional<real>;

    constexpr void get_surface_information(vec3 local_pt, vec2& uv, vec3& dpdu, vec3& dpdv) const;

    constexpr auto normal_at(vec3 pt) const -> vec3;

    constexpr auto material_index() const -> u32 { return m_mat_idx; }
};

static_assert(concepts::bound_shape<sphere>);

}// namespace trc::shapes

#include <tracer/shape/detail/sphere.ipp>
