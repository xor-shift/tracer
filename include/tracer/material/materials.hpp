#pragma once

#include <tracer/material/lambertian.hpp>
#include <tracer/material/oren_nayar.hpp>

namespace trc {

using material = std::variant<materials::lambertian, materials::frensel_conductor, materials::frensel_dielectric, materials::oren_nayar>;

}// namespace trc
