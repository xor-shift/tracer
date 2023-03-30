#pragma once

#include <tracer/shape/concepts.hpp>

#include <tracer/shape/disc.hpp>
#include <tracer/shape/sphere.hpp>
#include <tracer/shape/plane.hpp>

#include <variant>

namespace trc {

using bound_shape = std::variant<shapes::sphere, shapes::disc>;
using unbound_shape = std::variant<shapes::plane>;

using shape = std::variant<shapes::sphere, shapes::disc, shapes::plane>;

}
