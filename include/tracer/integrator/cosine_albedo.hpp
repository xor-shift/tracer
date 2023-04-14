#pragma once

#include <tracer/integrator/detail/pixel_integrator.hpp>

namespace trc {

struct cosine_albedo_integrator : detail::pixel_integrator {
    cosine_albedo_integrator(std::shared_ptr<camera> camera, std::shared_ptr<scene> scene)
        : pixel_integrator(std::move(camera), std::move(scene)) {}

protected:
    virtual auto kernel(vec2 xy, default_rng& gen) noexcept -> color final override {
        ray ray = m_camera->generate_ray(xy, gen);

        auto isect_res = m_scene->intersect(ray);
        if (!isect_res)
            return {};

        intersection isect = *isect_res;

        auto interaction = VARIANT_CALL(m_scene->material(isect.material_index), sample, isect, gen);
        auto const& [wi, wi_pdf, albedo, emittance, _] = interaction;

        return emittance + albedo * std::clamp<real>(std::abs(dot(isect.wo, isect.get_global_normal())) + 0.1, 0, 1);
    }
};

}// namespace trc
