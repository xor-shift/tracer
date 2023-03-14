#pragma once

#include <stuff/core.hpp>
#include <stuff/blas.hpp>

namespace trc {

using namespace stf::integers;

using real = double;

using bvec2 = stf::blas::vector<bool, 2>;
using bvec3 = stf::blas::vector<bool, 3>;
using bvec4 = stf::blas::vector<bool, 4>;
using ivec2 = stf::blas::vector<int, 2>;
using ivec3 = stf::blas::vector<int, 3>;
using ivec4 = stf::blas::vector<int, 4>;
using vec2 = stf::blas::vector<real, 2>;
using vec3 = stf::blas::vector<real, 3>;
using vec4 = stf::blas::vector<real, 4>;

using mat2x2 = stf::blas::matrix<real, 2, 2>;
using mat3x3 = stf::blas::matrix<real, 3, 3>;
using mat4x4 = stf::blas::matrix<real, 4, 4>;

using color = vec3;

}
