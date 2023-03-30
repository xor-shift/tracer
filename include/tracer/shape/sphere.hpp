#pragma once

#include <stuff/ranvec.hpp>
#include <stuff/expected.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <optional>

namespace trc::shapes {

struct sphere {
    vec3 center_point;
    real radius;
    u32 material_index;

    constexpr auto intersect(ray const& ray) const -> std::optional<intersection>;

    constexpr auto intersects(ray const& ray) const -> bool { return !!intersect_impl(ray); }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return {center_point - vec3(radius), center_point + vec3(radius)}; }

    constexpr auto center() const -> vec3 { return center_point; }

    template<typename Gen>
    constexpr auto sample_surface(Gen& gen) const -> intersection;

    constexpr auto normal_at(vec3 pt) const -> vec3;

    constexpr auto surface_area() const -> real {
        return 4 * std::numbers::pi_v<real> * radius;
    }

private:
    constexpr auto intersect_impl(ray const& ray) const -> std::optional<real>;

    constexpr void get_surface_information(vec3 local_pt, vec2& uv, vec3& dpdu, vec3& dpdv) const;
};

static_assert(concepts::bound_shape<sphere>);

}// namespace trc::shapes

#include <tracer/shape/detail/sphere.ipp>
