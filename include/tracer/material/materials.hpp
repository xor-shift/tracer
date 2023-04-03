#pragma once

#include <tracer/material/fresnel.hpp>
#include <tracer/material/lambertian.hpp>
#include <tracer/material/oren_nayar.hpp>

namespace trc {

using material = std::variant<materials::lambertian, materials::fresnel_conductor, materials::fresnel_dielectric, materials::oren_nayar>;

}// namespace trc
