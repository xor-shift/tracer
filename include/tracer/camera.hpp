#pragma once

#include <tracer/common.hpp>
#include <tracer/ray.hpp>

namespace trc {

struct camera {
    constexpr camera(vec2 dimensions) noexcept
        : m_dimensions(dimensions) {}

    virtual ~camera() = default;

    virtual constexpr auto generate_ray(vec2 xy, default_rng& gen) const -> ray = 0;

    constexpr auto dimensions() const -> vec2 { return m_dimensions; }

protected:
    vec2 m_dimensions;
};

}// namespace trc
