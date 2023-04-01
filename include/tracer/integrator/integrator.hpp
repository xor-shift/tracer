#pragma once

#include <stuff/core.hpp>

#include <tracer/camera.hpp>
#include <tracer/common.hpp>
#include <tracer/image.hpp>
#include <tracer/scene.hpp>

#include <chrono>
#include <memory>

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

}
