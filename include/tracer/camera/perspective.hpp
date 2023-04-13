#pragma once

#include <tracer/camera.hpp>

namespace trc {

struct frustum_camera : camera {
    constexpr frustum_camera(vec2 dimensions, real left, real right, real bottom, real top, real near, real far) noexcept
        : camera(dimensions)
        , m_transform({}) {
    }

    constexpr auto generate_ray(vec2 xy, default_rng& gen) const -> ray override {
        return {};
    }

private:
    mat4x4 m_transform;
};

}// namespace trc
