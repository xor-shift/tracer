#pragma once

#include <tracer/common.hpp>
#include <tracer/ray.hpp>

namespace trc {

struct intersection {
    constexpr intersection() = default;
    constexpr intersection(intersection const&) = default;
    constexpr intersection(intersection&&) = default;
    constexpr intersection& operator=(intersection const&) = default;
    constexpr intersection& operator=(intersection&&) = default;

    constexpr intersection(u32 mat_idx, vec3 wo, real t, vec3 isection_point, vec2 uv, std::pair<vec3, vec3> dpduv)
        : wo(wo)
        , t(t)
        , isection_point(isection_point)
        , uv(uv)
        , material_index(mat_idx) {
        set_surface_derivatives(dpduv);
        local_wo = vector_to_refl_space(wo);
    }

    // old style, remove
    constexpr intersection(std::in_place_t, u32 mat_idx, vec3 wo, real t, vec3 isection_point, vec2 uv, vec3 normal)
        : wo(wo)
        , t(t)
        , isection_point(isection_point)
        , uv(uv)
        , normal(normal)
        , material_index(mat_idx) {
    }

    constexpr void set_surface_derivatives(std::pair<vec3, vec3> dpduv) {
        this->dpduv = dpduv;
        st.first = normalize(dpduv.first);
        normal = normalize(cross(dpduv.first, dpduv.second));
        st.second = cross(normal, st.first);
    }

    constexpr auto vector_to_refl_space(vec3 vec) const -> vec3 {
        auto const& [s, t] = st;

        //mat3x3 mat{s[0], s[1], s[2], t[0], t[1], t[2], normal[0], normal[1], normal[2]};
        //return mat * vec;

        return vec3{dot(vec, s), dot(vec, t), dot(vec, normal)};
    }

    constexpr auto vector_from_refl_space(vec3 vec) const -> vec3 {
        // orthonormal matrices' transposes are their inverses

        auto const& [s, t] = st;

        //mat3x3 mat{s[0], t[0], normal[0], s[1], t[1], normal[1], s[2], t[2], normal[2]};
        //return mat * vec;

        return vec3{
          s[0] * vec[0] + t[0] * vec[1] + normal[0] * vec[2],
          s[1] * vec[0] + t[1] * vec[1] + normal[1] * vec[2],
          s[2] * vec[0] + t[2] * vec[1] + normal[2] * vec[2],
        };
    }

    constexpr auto get_global_wo() const -> vec3 { return wo; }
    constexpr auto get_local_wo() const -> vec3 { return local_wo; }

    constexpr auto get_global_normal() const -> vec3 { return normal; }
    constexpr auto get_local_normal() const -> vec3 { return vec3{0, 0, 1}; }

    constexpr auto cosine_theta(vec3 vec_l) const -> real { return vec_l[2]; }
    constexpr auto cosine_sq_theta(vec3 vec_l) const -> real { return vec_l[2] * vec_l[2]; }
    constexpr auto sin_theta(vec3 vec_l) const -> real { return std::sqrt(sin_sq_theta(vec_l)); }
    constexpr auto sin_sq_theta(vec3 vec_l) const -> real { return 1 - cosine_sq_theta(vec_l); }

    constexpr auto cosine_phi(vec3 vec_l) const -> real {
        real sin = sin_theta(vec_l);
        return sin == 0 ? 1 : vec_l[0] / sin;
    }

    constexpr auto sin_phi(vec3 vec_l) const -> real {
        real sin = sin_theta(vec_l);
        return sin == 0 ? 1 : vec_l[1] / sin;
    }

    vec3 wo;
    vec3 local_wo;
    real t;

    vec3 isection_point;        // global
    vec2 uv;                    // parametric
    std::pair<vec3, vec3> dpduv;// global
    vec3 normal;                // global
    std::pair<vec3, vec3> st;   // global

    u32 material_index;
};

struct interaction {
    vec3 wi;
    real wi_prob;

    color attenuation;
    color emittance;

    intersection isection;
};

}// namespace trc
