#pragma once

#include <tracer/integrator/detail/task_integrator.hpp>

namespace trc::detail {

struct pixel_integrator : task_integrator<> {
    pixel_integrator(std::shared_ptr<camera> camera, std::shared_ptr<scene> scene, task_generator_type const& generator = {})
        : task_integrator<>(std::move(camera), std::move(scene), generator) {}

protected:
    virtual constexpr auto kernel(vec2 xy, default_rng& gen) noexcept -> color = 0;

    constexpr void task_processor(task_payload_type payload, image_like& out, integration_settings opts, default_rng& gen) noexcept final override {
        stf::random::erand48_distribution<real> dist{};

        vec2 dims(out.width(), out.height());

        usize upto_row = std::min(payload.xy_start.second + payload.span.second, out.height());
        usize upto_col = std::min(payload.xy_start.first + payload.span.first, out.width());

        for (usize row = payload.xy_start.second; row < upto_row; row++) {
            for (usize col = payload.xy_start.first; col < upto_col; col++) {
                isize flipped_row = static_cast<isize>(out.height() - row) - 1;
                vec2 cr_start = vec2(col, flipped_row) - (dims / 2);

                color sum{};

                for (usize i = 0; i < opts.samples; i++) {
                    vec2 sample = vec2(dist(gen), dist(gen));

                    sum = sum + kernel(cr_start + sample, gen);
                }

                out.set(col, row, sum / static_cast<real>(opts.samples));
            }
        }
    }
};

}
