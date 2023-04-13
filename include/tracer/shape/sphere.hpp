#pragma once

#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>
#include <tracer/shape/shape.hpp>
#include <tracer/shape/detail/util.hpp>

#include <stuff/expected.hpp>
#include <stuff/ranvec.hpp>

#include <optional>

namespace trc::shapes {

struct sphere {
    constexpr sphere() = default;

    constexpr sphere(u32 mat_idx, vec3 center, real radius) noexcept
        : m_center(center)
        , m_radius(radius)
        , m_mat_idx(mat_idx) {}

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection>;
    
    constexpr auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> {
        ++stats.shape_intersection_tests;
        return intersect(ray, best_t);
    }

    constexpr auto intersects(ray const& ray) const -> bool { return !!intersect_impl(ray); }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return {m_center - vec3(m_radius), m_center + vec3(m_radius)}; }

    constexpr auto center() const -> vec3 { return m_center; }

    template<typename Gen>
    constexpr auto sample_surface(Gen& gen) const -> intersection;

    constexpr auto surface_area() const -> real {
        return 4 * std::numbers::pi_v<real> * m_radius;
    }

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

    constexpr void set_material(u32 idx) { m_mat_idx = idx; }

    friend constexpr void swap(sphere&, sphere&);

private:
    vec3 m_center;
    real m_radius;
    u32 m_mat_idx;

    constexpr auto intersect_impl(ray const& ray) const -> std::optional<real>;

    constexpr void get_surface_information(vec3 local_pt, vec2& uv, vec3& dpdu, vec3& dpdv) const;

    constexpr auto normal_at(vec3 pt) const -> vec3;
};

constexpr void swap(sphere& lhs, sphere& rhs) {
    using std::swap;

    swap(lhs.m_center, rhs.m_center);
    swap(lhs.m_radius, rhs.m_radius);
    swap(lhs.m_mat_idx, rhs.m_mat_idx);
}

static_assert(concepts::bound_shape<sphere>);

}// namespace trc::shapes

#include <tracer/shape/detail/sphere.ipp>
