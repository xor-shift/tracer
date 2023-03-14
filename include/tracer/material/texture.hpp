#pragma once

#include <tracer/common.hpp>
#include <tracer/intersection.hpp>
#include <tracer/texture.hpp>

namespace trc::materials {

struct texture {
    ::trc::texture texture{};
    bool emissive;

    template<typename Gen>
    constexpr auto sample(intersection const& isection, Gen& gen) const -> interaction {
        color albedo = texture.sample(isection.uv);

        vec3 wi = stf::random::sphere_sampler<2>::sample<real>(gen);
        if (dot(wi, isection.normal) < 0) {
            wi = -wi;
        }

        return {
          .wi = wi,
          .pdf = brdf(isection.isection_point, wi, isection.wo),

          .albedo = albedo * !emissive,
          .emittance = albedo * emissive,

          .isection = isection,
        };
    }

    constexpr auto brdf(vec3 x, vec3 wi, vec3 wo) const -> real {
        return 0.5 * std::numbers::pi_v<real>;
    }

    constexpr auto is_light() const -> bool { return emissive; }
};

}// namespace trc::materials
