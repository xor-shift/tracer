#pragma once

#include <tracer/material/material.hpp>

namespace trc::materials {

struct oren_nayar : material_base {
    constexpr oren_nayar(real sigma, albedo_source albedo, albedo_source emission = color(0)) noexcept
        : material_base(albedo, emission)
        , m_sigma(sigma) {}

    template<typename Gen>
    constexpr auto sample(intersection const& isection, Gen& gen) const -> interaction {
        vec3 wi = stf::random::sphere_sampler<2>::sample<real>(gen);
        real pdf = std::numbers::inv_pi_v<real> * real(0.5);
        if (dot(wi, isection.get_local_normal()) < 0) {
            wi = -wi;
        }

        return {
          .wi = isection.vector_from_refl_space(wi),
          .wi_prob = pdf,

          .attenuation = albedo_at(isection.uv) * brdf(isection, wi, isection.get_local_wo()),
          .emittance = le_at(isection.uv),

          .isection = isection,
        };
    }

    constexpr auto fn(intersection const& isection, vec3 wi_local) const -> interaction {
        return interaction{
          .wi = isection.vector_from_refl_space(wi_local),

          .attenuation = albedo_at(isection.uv) * brdf(isection, wi_local, isection.get_local_wo()),
          .emittance = le_at(isection.uv),

          .isection = isection,
        };
    }

    constexpr auto brdf(intersection const& isection, vec3 wi_local, vec3 wo_local) const -> real {
        real σ = m_sigma;
        real σ2 = σ * σ;

        real sinθ_i = isection.sin_theta(wi_local);
        real sinθ_o = isection.sin_theta(wo_local);

        real a = 1 - σ2 / (2 * (σ2 + 0.33));
        real b = 0.45 * σ2 / (σ2 + 0.09);
        real α = std::max(sinθ_i, sinθ_o);
        real β = std::min(sinθ_i, sinθ_o);

        real φ_i = std::asin(isection.sin_phi(wi_local));
        real φ_o = std::asin(isection.sin_phi(wo_local));

        return std::numbers::inv_pi_v<real> * (a + b * std::max<real>(0, std::cos(φ_i - φ_o)) * std::sin(α) * std::tan(β));
    }

private:
    real m_sigma;
};

}
