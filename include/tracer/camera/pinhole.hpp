#pragma once

#include <tracer/camera.hpp>

namespace trc {

struct pinhole_camera : camera {
    constexpr pinhole_camera(vec3 center, vec2 dimensions, real fov, mat3x3 transform = stf::blas::identity_matrix<real, 3, stf::blas::matrix>()) noexcept
        : camera(dimensions)
        , m_center(center)
        , m_transform(transform) {
        set_fov(fov);
    }

    constexpr void set_fov(real fov) {
        m_fov = fov;

        real theta = fov / 2;
        m_d = (1 / (2 * std::sin(theta))) * std::sqrt(std::abs(m_dimensions[0] * (2 - m_dimensions[0])));
    }

    constexpr auto generate_ray(vec2 xy, default_rng& gen) const -> ray override {
        return ray(m_center, m_transform * normalize(vec3(xy, m_d)));
    }

private:
    vec3 m_center;
    mat3x3 m_transform;
    real m_fov;
    real m_d;
};

}// namespace trc
