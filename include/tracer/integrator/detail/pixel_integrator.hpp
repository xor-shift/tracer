#pragma once

#include <tracer/integrator/detail/task_integrator.hpp>

namespace trc::detail {

struct pixel_integrator : task_integrator<> {
    pixel_integrator(std::shared_ptr<camera> camera, std::shared_ptr<scene> scene, task_generator_type const& generator = {})
        : task_integrator<>(std::move(camera), std::move(scene), generator) {}

protected:
    virtual constexpr auto kernel(vec2 xy, default_rng& gen) noexcept -> color = 0;

    constexpr void task_processor(task_payload_type payload, integration_options opts, default_rng& gen) noexcept final override {
        stf::random::erand48_distribution<real> dist{};

        vec2 dims(payload.image.width(), payload.image.height());

        for (usize local_row = 0; local_row < payload.chunk.height(); local_row++) {
            for (usize local_col = 0; local_col < payload.chunk.width(); local_col++) {
                usize row = local_row + payload.xy_start.second;
                usize col = local_col + payload.xy_start.first;

                isize flipped_row = static_cast<isize>(payload.image.height() - row) - 1;
                vec2 cr_start = vec2(col, flipped_row) - (dims / 2);

                color sum{};

                for (usize i = 0; i < opts.samples; i++) {
                    vec2 sample = vec2(dist(gen), dist(gen));

                    sum = sum + kernel(cr_start + sample, gen);
                }

                payload.chunk.at(local_col, local_row) = sum / static_cast<real>(opts.samples);
            }
        }
    }
};

}
