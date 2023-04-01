#pragma once

#include <tracer/common.hpp>

namespace trc::detail {

/// A function that can be used when deriving surface derivatives is a WIP/TO-DO.\n
/// This function is very slow and should, generally, not be used.
/// @return
/// Two vectors that are orthogonal to both the <code>param</code> vector and to each other.
constexpr auto get_dummy_dp_duv(vec3 normal) -> std::pair<vec3, vec3> {
    assert_normal(normal);

    /*vec3 a = cross(normal, vec3(1, 0, 0));
    vec3 b = cross(normal, vec3(0, 1, 0));
    vec3 c = cross(normal, vec3(0, 0, 1));

    vec3 max_ab = dot(a, a) <= dot(b, b) ? b : a;
    vec3 max_abc = dot(max_ab, max_ab) <= dot(c, c) ? c : max_ab;

    vec3 du_dp = normalize(max_abc);
    vec3 dv_dp = cross(normal, du_dp);

    return {du_dp, dv_dp};*/

    vec3 cross_vec;

    if (normal[1] == 0 && normal[2] == 0) {
        cross_vec = {0, 1, 0};
    } else {
        cross_vec = {1, 0, 0};
    }

    vec3 du_dp = cross(cross_vec, normal);
    vec3 dv_dp = cross(normal, du_dp);

    return {du_dp, dv_dp};
}

constexpr auto get_sphere_uv(vec3 pt, real radius, std::pair<real, real> theta_range) -> std::pair<vec2, std::pair<real, real>> {
    real phi = std::atan2(pt[1], pt[0]);
    if (phi < 0) phi += 2 * std::numbers::pi_v<real>;
    real theta = std::acos(std::clamp<real>(pt[2] / radius, -1, 1));

    real u = phi * real(0.5) * std::numbers::inv_pi_v<real>;
    real v = (theta - theta_range.first) / (theta_range.second - theta_range.first);

    return {{u, v}, {phi, theta}};
}

}
