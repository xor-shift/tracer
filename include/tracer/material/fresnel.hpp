#pragma once

#include <tracer/material/material.hpp>

namespace trc::fresnel {

constexpr auto reflectance_schlick(real cos_theta_i, real n_1, real n_2) -> real {
    real c = 1 - cos_theta_i;
    real r_0_sq = (n_1 - n_2) / (n_1 + n_2);

    real r_0 = r_0_sq * r_0_sq;

    return r_0 + (1 - r_0) * (c * c * c * c * c);
}

constexpr auto reflectance_fresnel(real cos_theta_i, real n_1, real n_2) -> real {
    real n_1_c_i = n_1 * cos_theta_i;
    real n_2_c_i = n_2 * cos_theta_i;
    real n_1_c_t = n_1 * std::sqrt(1 - cos_theta_i * cos_theta_i);
    real n_2_c_t = n_2 * std::sqrt(1 - cos_theta_i * cos_theta_i);

    real r_s_sq = (n_2_c_i - n_1_c_t) / (n_2_c_i + n_1_c_t);
    real r_p_sq = (n_2_c_t - n_1_c_i) / (n_2_c_t + n_1_c_i);

    return real(0.5) * (r_s_sq * r_s_sq + r_p_sq * r_p_sq);
}

constexpr auto reflect(vec3 v, vec3 n) -> vec3 {
    return -v + 2 * dot(n, v) * n;
}

constexpr auto refract(vec3 v, vec3 n, real n_ratio) -> std::optional<vec3> {
    real cosθ_i = dot(n, v);
    real sin2θ_i = 1 - cosθ_i * cosθ_i;
    real sin2θ_t = n_ratio * n_ratio * sin2θ_i;

    if (sin2θ_t >= 1)
        return std::nullopt;

    real cosθ_t = std::sqrt(1 - sin2θ_t);
    return n_ratio * -v + (n_ratio * cosθ_i - cosθ_t) * n;
}

}// namespace trc::fresnel

namespace trc::materials {

struct fresnel_conductor : material_base {
    constexpr fresnel_conductor(albedo_source albedo, albedo_source emission = color(0)) noexcept
        : material_base(albedo, emission) {}

    template<typename Gen>
    constexpr auto sample(intersection const& isection, Gen& gen) const -> interaction {
        vec3 refl_wi = fresnel::reflect(isection.get_global_wo(), isection.get_global_normal());

        return {
          .wi = refl_wi,
          .wi_prob = 1,

          .attenuation = albedo_at(isection) / std::abs(dot(refl_wi, isection.get_global_normal())),
          .emittance = le_at(isection),

          .isection = isection,
        };
    }

    constexpr auto fn(intersection const& isection, vec3 wi_local) const -> interaction {
        return interaction{
          .wi = isection.vector_from_refl_space(wi_local),

          .attenuation = color(0),
          .emittance = le_at(isection),

          .isection = isection,
        };
    }

    constexpr auto brdf(intersection const& isection, vec3 wi_local, vec3 wo_local) const -> real {
        return 0;
    }
};

struct fresnel_dielectric : material_base {
    constexpr fresnel_dielectric(real n_i, real n_t, albedo_source albedo, albedo_source emission = color(0)) noexcept
        : material_base(albedo, emission)
        , m_n_i(n_i)
        , m_n_t(n_t) {}

    template<typename Gen>
    constexpr auto sample(intersection const& isection, Gen& gen) const -> interaction {
        stf::random::erand48_distribution dist{};
        real rand_num = dist(gen);

        real n_i = m_n_i;
        real n_t = m_n_t;
        real cos_theta_i = std::abs(isection.cosine_theta(isection.get_local_wo()));
        vec3 normal = isection.get_global_normal();
        vec3 wo = isection.get_global_wo();

        if (isection.cosine_theta(isection.get_local_wo()) < 0) {
            std::swap(n_i, n_t);
            normal = -normal;
        }

        real reflectivity = fresnel::reflectance_schlick(cos_theta_i, n_i, n_t);
        real pdf;
        vec3 wi;

        if (rand_num < reflectivity) {
            if (rand_num < reflectivity) {
                wi = fresnel::reflect(wo, normal);
                pdf = reflectivity;
            }
        } else {
            if (auto res = fresnel::refract(wo, normal, n_i / n_t); res) {
                wi = *res;
                pdf = 1 - reflectivity;
            } else {
                wi = fresnel::reflect(wo, normal);
                pdf = 1;
            }
        }


        return {
          .wi = wi,
          .wi_prob = pdf,

          .attenuation = pdf * albedo_at(isection) / std::abs(dot(wi, normal)),
          .emittance = le_at(isection),

          .isection = isection,
        };
    }

    constexpr auto fn(intersection const& isection, vec3 wi_local) const -> interaction {
        return interaction{
          .wi = isection.vector_from_refl_space(wi_local),

          .attenuation = color(0),
          .emittance = le_at(isection),

          .isection = isection,
        };
    }

    constexpr auto brdf(intersection const& isection, vec3 wi_local, vec3 wo_local) const -> real {
        return 0;
    }

private:
    real m_n_i;
    real m_n_t;
};

}
