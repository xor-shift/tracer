#pragma once

#include <tracer/common.hpp>

namespace trc {

enum class color_map_gradient : int {
    magma = 0,
    inferno = 1,
    plasma = 2,
    viridis = 3,
    cividis = 4,
    twilight = 5,
    turbo = 6,
    jet,
};

constexpr static auto color_map(real param, color_map_gradient grad = color_map_gradient::plasma) -> color;

}// namespace trc

#include <tracer/detail/colormap.ipp>
