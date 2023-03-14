#pragma once

#include <tracer/common.hpp>
#include <tracer/intersection.hpp>

namespace trc::materials {

struct simple_color {
    color color;
    bool emissive;

    template<typename Gen>
    constexpr auto sample(intersection const& isection, Gen& gen) const -> interaction {
        vec3 wi = stf::random::sphere_sampler<2>::sample<real>(gen);
        if (dot(wi, isection.normal) < 0) {
            wi = -wi;
        }

        return {
          .wi = wi,
          .pdf = brdf(isection.isection_point, wi, isection.wo),

          .albedo = color * !emissive,
          .emittance = color * emissive,

          .isection = isection,
        };
    }

    constexpr auto brdf(vec3 x, vec3 wi, vec3 wo) const -> real {
        return 0.5 * std::numbers::pi_v<real>;
    }

    constexpr auto is_light() const -> bool { return emissive; }
};

}
