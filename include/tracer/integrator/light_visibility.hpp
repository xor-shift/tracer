#pragma once

#include <tracer/integrator/detail/pixel_integrator.hpp>

namespace trc {

struct light_visibility_checker : detail::pixel_integrator {
    light_visibility_checker(std::shared_ptr<camera> camera, std::shared_ptr<scene> scene)
        : pixel_integrator(std::move(camera), std::move(scene)) {}

protected:
    virtual constexpr auto kernel(vec2 xy, default_rng& gen) noexcept -> color final override {
        ray ray = m_camera->generate_ray(xy, gen);

        intersection isect;
        if (std::optional<intersection> isect_res = m_scene->intersect(ray); !isect_res) {
            return color{0, 0, 0};
        } else {
            isect = *isect_res;
        }

        bound_shape picked_shape = m_scene->pick_light(gen);

        intersection sample = VARIANT_CALL(picked_shape, sample_surface, gen);

        bool hit = m_scene->visibility_check(isect.isection_point + isect.normal * 0.0001, sample.isection_point + sample.normal * 0.0001);

        return hit ? color(VARIANT_CALL(picked_shape, surface_area)) : color(0);
    }
};

}
