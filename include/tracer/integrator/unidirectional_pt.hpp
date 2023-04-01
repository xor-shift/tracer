#pragma once

#include <tracer/integrator/detail/pixel_integrator.hpp>

namespace trc {

struct unidirectional_pt : detail::pixel_integrator {
    unidirectional_pt(std::shared_ptr<camera> camera, std::shared_ptr<scene> scene)
        : pixel_integrator(std::move(camera), std::move(scene)) {}

protected:
    virtual constexpr auto kernel(vec2 xy, default_rng& gen) noexcept -> color final override {
        ray ray = m_camera->generate_ray(xy, gen);

        constexpr usize depth_threshold = 3;
        constexpr real base_rr_prob = 0.05;

        color attenuation(1);
        color light{};

        for (usize depth = 0;; depth++) {
            auto isect_res = m_scene->intersect(ray);
            if (!isect_res)
                break;

            intersection isect = *isect_res;

            auto interaction = VARIANT_CALL(m_scene->material(isect.material_index), sample, isect, gen);
            auto const& [wi, wi_pdf, albedo, emittance, _] = interaction;

            ray = {isect.isection_point + interaction.wi * 0.001, interaction.wi};

            light = light + emittance * attenuation;

            real cos_weight = std::abs(dot(wi, isect.normal));
            real weight = cos_weight / wi_pdf;
            color cur_attenuation = weight * albedo;
            attenuation = cur_attenuation * attenuation;

            if (const auto abs_attenuation = abs(attenuation); depth > depth_threshold && abs_attenuation < 0.2) {
                stf::random::erand48_distribution<real> dist{};
                real rr_gen = dist(gen);

                // probability to continue
                real prob = std::max(base_rr_prob, std::clamp<real>(abs_attenuation, 0, 1));

                if (rr_gen < prob) {
                    attenuation = attenuation / (1 - prob);
                } else {
                    break;
                }
            }
        }

        return light;
    }
};

}
