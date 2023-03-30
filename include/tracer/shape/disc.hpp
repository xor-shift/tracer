#pragma once

#include <stuff/ranvec.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>
#include <tracer/shape/plane.hpp>

#include <optional>

namespace trc::shapes {

struct disc {
    vec3 center_point;
    vec3 normal;
    real radius;
    u32 material_index;

    constexpr auto intersect(ray const& ray) const -> std::optional<intersection>;

    constexpr auto intersects(ray const& ray) const -> bool;

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return {center_point - vec3(radius), center_point + vec3(radius)}; }

    constexpr auto center() const -> vec3 { return center_point; }

    template<typename Gen>
    constexpr auto sample_surface(Gen& gen) const -> intersection;

    constexpr auto normal_at(vec3 pt) const -> vec3 { return normal; }

    constexpr auto surface_area() const -> real {
        return std::numbers::pi_v<real> * radius * radius;
    }

private:
    constexpr auto get_surface_basis() const -> std::pair<vec3, vec3>;

    constexpr void get_surface_information(vec3 r_p, vec2& uv, vec3& dpdu, vec3& dpdv) const;
};

static_assert(concepts::bound_shape<disc>);

}// namespace trc::shapes

#include <tracer/shape/detail/disc.ipp>
