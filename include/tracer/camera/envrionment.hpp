#pragma once

#include <tracer/camera.hpp>

namespace trc {

struct environment_camera : camera {
    constexpr environment_camera(vec3 center, vec2 dimensions) noexcept
        : camera(dimensions)
        , m_center(center) {}

    constexpr auto generate_ray(vec2 xy, default_rng& gen) const -> ray override {
        vec2 uv = xy / m_dimensions + vec2{0.25, 0.5};

        real theta = std::numbers::pi_v<real> * uv[1];
        real phi = 2 * std::numbers::pi_v<real> * uv[0];

        //real theta = uv[0] * 2 * std::numbers::pi_v<real>;
        //real phi = std::acos(2 * uv[1] - 1);

        real st = std::sin(theta);
        real ct = std::cos(theta);
        real sp = std::sin(phi);
        real cp = std::cos(phi);

        vec3 dir(st * cp, -ct, st * sp);

        return ray(m_center, dir);
    }

private:
    vec3 m_center;
};

}// namespace trc
