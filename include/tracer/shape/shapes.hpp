#pragma once

#include <tracer/shape/shape.hpp>

#include <tracer/shape/box.hpp>
#include <tracer/shape/disc.hpp>
#include <tracer/shape/sphere.hpp>
#include <tracer/shape/plane.hpp>

#include <variant>

namespace trc {

using bound_shape = std::variant<shapes::sphere, shapes::disc, shapes::box>;
using unbound_shape = std::variant<shapes::plane>;

using shape = std::variant<shapes::sphere, shapes::disc, shapes::box, shapes::plane>;

}
