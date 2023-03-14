#pragma once

#include <tracer/common.hpp>

namespace trc {

struct ray {
    constexpr ray() = default;

    constexpr ray(vec3 origin, vec3 direction)
        : origin(origin)
        , direction(direction)
        , direction_reciprocals(1 / direction) {}

    vec3 origin;
    vec3 direction;
    vec3 direction_reciprocals;
};

}// namespace trc
