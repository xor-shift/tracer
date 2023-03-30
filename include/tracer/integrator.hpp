#pragma once

#include <stuff/thread.hpp>

#include <tracer/camera.hpp>
#include <tracer/image.hpp>
#include <tracer/scene.hpp>

#include <chrono>
#include <memory>
#include <thread>

namespace trc {

struct integration_options {
    usize samples = 0;
    std::chrono::seconds sample_for = std::chrono::years(1);
};

struct integrator {
    integrator(std::shared_ptr<camera> camera, std::shared_ptr<scene> scene)
        : m_camera(std::move(camera))
        , m_scene(std::move(scene)) {}

    virtual ~integrator() noexcept = default;

    virtual constexpr void integrate(image_view out, integration_options opts, default_rng& gen) noexcept = 0;

protected:
    std::shared_ptr<camera> m_camera{nullptr};
    std::shared_ptr<scene> m_scene{nullptr};
};

namespace detail {

struct default_task {
    std::pair<usize, usize> xy_start;
    image_view chunk;
    image_view image;
};

struct default_task_generator {
    using payload_type = default_task;

    constexpr default_task_generator() = default;

    constexpr default_task_generator(std::pair<usize, usize> chunk_size) noexcept
        : m_chunk_size(chunk_size) {}

    constexpr void set_image(image_view image) {
        m_image = image;
        m_x_tasks = image.width() / m_chunk_size.first + (image.width() % m_chunk_size.first != 0);
        m_y_tasks = image.height() / m_chunk_size.second + (image.height() % m_chunk_size.second != 0);
    }

    constexpr auto n_tasks() const -> usize {
        return m_x_tasks * m_y_tasks;
    }

    constexpr auto operator()(usize task) const -> payload_type {
        usize start_col = (task % m_x_tasks) * m_chunk_size.first;
        usize start_row = (task / m_x_tasks) * m_chunk_size.second;

        image_view image(m_image, start_col, start_row, m_chunk_size.first, m_chunk_size.second);

        return default_task{
          .xy_start{start_col, start_row},
          .chunk = image,
          .image = m_image,
        };
    }

private:
    std::pair<usize, usize> m_chunk_size{16, 16};

    image_view m_image{};
    usize m_x_tasks = 0;
    usize m_y_tasks = 0;
};

}// namespace detail

template<typename TaskGenerator = detail::default_task_generator>
struct task_integrator : integrator {
    using task_generator_type = TaskGenerator;
    using task_payload_type = typename TaskGenerator::payload_type;

    task_integrator(std::shared_ptr<camera> camera, std::shared_ptr<scene> scene, task_generator_type const& generator = {})
        : integrator(camera, scene)
        , m_task_generator(generator) {}

    constexpr void integrate(image_view out, integration_options opts, default_rng& gen) noexcept final override {
        m_task_generator.set_image(out);

        usize n_threads;

        if consteval {
            n_threads = 1;
        } else {
#ifdef NDEBUG
            if ((n_threads = std::thread::hardware_concurrency()) == 0) {
                n_threads = 1;
            }
#else
            n_threads = 1;
#endif
        }

        if (n_threads <= 1) {
            for (usize n_tasks = m_task_generator.n_tasks(), i = 0; i < n_tasks; i++) {
                this->task_processor(m_task_generator(i), opts, gen);
            }

            return;
        }

        stf::wait_group wg{};
        std::vector<std::thread> workers{};

        auto worker = [this, &wg, out, opts](usize thread_id, usize stride, default_rng gen) {
            stf::scope::scope_exit wg_guard{[&wg] { wg.done(); }};

            for (usize n_tasks = m_task_generator.n_tasks(), i = thread_id; i < n_tasks; i += stride) {
                task_processor(m_task_generator(i), opts, gen);
            }
        };

        for (usize i = 0; i < n_threads; i++) {
            wg.add(1);

            default_rng thread_rng = gen;
            gen.jump_128();

            workers.emplace_back(worker, i, n_threads, thread_rng);
        }

        // this is not entirely necessary
        // keeping it just in case i switch away from std::thread in the future
        wg.wait();

        for (auto& w: workers) {
            w.join();
        }
    }

protected:
    virtual constexpr void task_processor(task_payload_type payload, integration_options opts, default_rng& gen) noexcept = 0;

private:
    task_generator_type m_task_generator;
};

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

struct unidirectional_pt : pixel_integrator {
    unidirectional_pt(std::shared_ptr<camera> camera, std::shared_ptr<scene> scene)
        : pixel_integrator(std::move(camera), std::move(scene)) {}

protected:
    virtual constexpr auto kernel(vec2 xy, default_rng& gen) noexcept -> color final override {
        ray ray = m_camera->generate_ray(xy, gen);

        constexpr usize depth_threshold = 3;
        constexpr real base_rr_prob = 0.05;

        color attenuation(1);
        color light {};

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
                stf::random::erand48_distribution<real> dist {};
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

struct light_visibility_checker : pixel_integrator {
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

}// namespace trc
