#pragma once

#include <tracer/shape/shape.hpp>

#include <tracer/shape/box.hpp>
#include <tracer/shape/disc.hpp>
#include <tracer/shape/mesh.hpp>
#include <tracer/shape/plane.hpp>
#include <tracer/shape/sphere.hpp>
#include <tracer/shape/triangle.hpp>

#include <variant>

namespace trc {

using bound_shape = std::variant<shapes::sphere, shapes::disc, shapes::box, shapes::triangle, shapes::mesh<u16>, shapes::mesh<u32>>;
using unbound_shape = std::variant<shapes::plane>;

using shape = std::variant<shapes::sphere, shapes::disc, shapes::box, shapes::triangle, shapes::mesh<u16>, shapes::mesh<u32>, shapes::plane>;

static_assert(concepts::shape<dyn_shape<bound_shape>>);

}// namespace trc
