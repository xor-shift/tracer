#pragma once

#include <tracer/camera.hpp>

namespace trc {

struct orthographic_camera : camera {
    constexpr orthographic_camera(vec2 dimensions, vec3 center, real left, real right, real bottom, real top, real near, real far, vec3 plane_rot, vec3 ray_rot) noexcept
        : camera(dimensions) {
        m_transform = mat4x4::rotate(plane_rot[0], plane_rot[1], plane_rot[2]) *                                                   //
                      mat4x4::scale((right - left) / (2 * dimensions[0]), (top - bottom) / (2 * dimensions[1]), (far - near) / 2) *//
                      mat4x4::translate((left + right) / 2 + center[0], (top + bottom) / 2 + center[1], (far + near) / 2 + center[2]);

        m_transform_ray_rot = mat3x3::rotate(ray_rot[0], ray_rot[1], ray_rot[2]);
    }

    constexpr auto generate_ray(vec2 xy, default_rng& gen) const -> ray override {
        vec4 pt = m_transform * vec4(xy, 0, 1);
        vec4 dir = m_transform * vec4(0, 0, 1, 0);
        return ray((vec3(pt) / pt[3]), m_transform_ray_rot * normalize(vec3(dir)));
    }

private:
    mat4x4 m_transform;
    mat3x3 m_transform_ray_rot;
};

}
