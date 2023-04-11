#pragma once

#include <stuff/ranvec.hpp>

#include <tracer/material/material.hpp>

namespace trc::materials {

struct lambertian : material_base {
    constexpr lambertian(albedo_source albedo, albedo_source emission = color(0)) noexcept
        : material_base(albedo, emission) {}

    template<typename Gen>
    constexpr auto sample(intersection const& isection, Gen& gen) const -> interaction {
        auto [wi, pdf] = stf::random::cos_sphere_sampler<2>::polar<real>(gen);
        if (dot(wi, vec3{0, 0, 1}) < 0) {
            wi = -wi;
        }

        return {
          .wi = isection.vector_from_refl_space(wi),
          .wi_prob = pdf,

          .attenuation = albedo_at(isection) * brdf(isection, wi, isection.get_local_wo()),
          .emittance = le_at(isection),

          .isection = isection,
        };
    }

    constexpr auto fn(intersection const& isection, vec3 wi_local) const -> interaction {
        trc::color albedo = albedo_at(isection);

        return interaction{
          .wi = isection.vector_from_refl_space(wi_local),

          .attenuation = albedo_at(isection) * brdf(isection, wi_local, isection.get_local_wo()),
          .emittance = le_at(isection),

          .isection = isection,
        };
    }

    constexpr auto brdf(intersection const& isection, vec3 wi_local, vec3 wo_local) const -> real {
        return real(0.5) * std::numbers::inv_pi_v<real>;
    }
};

}// namespace trc::materials
