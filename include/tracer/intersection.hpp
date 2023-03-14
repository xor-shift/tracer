#pragma once

#include <tracer/common.hpp>
#include <tracer/ray.hpp>

namespace trc {

struct intersection {
    vec3 isection_point;
    vec2 uv;

    vec3 wo;
    real t;

    vec3 normal;
    u32 material_index;
};

struct interaction {
    vec3 wi;
    real pdf;

    color albedo;
    color emittance;

    intersection isection;
};

}
