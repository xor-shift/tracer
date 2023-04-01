#pragma once

#include <stuff/blas.hpp>
#include <stuff/core.hpp>

namespace trc {

using namespace stf::integers;

using real = double;

static constexpr real epsilon = 0.0001;

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

using default_rng = stf::random::xoshiro_256p;

constexpr void assert_no_nans(vec3 vec) noexcept {
#ifdef NDEBUG
    return;
#endif

    for (usize i = 0; i < 3; i++) {
        if (!std::isnan(vec[i])) {
            continue;
        }

        stf::unreachable_with_message<"vector contains NaN(s)">();
    }
}

constexpr void assert_normal(vec3 vec) noexcept {
#ifdef NDEBUG
    return;
#endif

    assert_no_nans(vec);

    real mag = abs(vec);
    real error = std::abs(mag) - 1;

    if (error < epsilon) {
        return;
    }

    stf::unreachable_with_message<"vector is not normalized">();
}

constexpr void assert_not_zero(vec3 vec) noexcept {
#ifdef NDEBUG
    return;
#endif

    assert_no_nans(vec);

    real mag = abs(vec);
    real error = std::abs(mag);

    if (error > epsilon) {
        return;
    }

    stf::unreachable_with_message<"vector is zero">();
}

constexpr void assert_orthogonal(vec3 v_0, vec3 v_1) noexcept {
#ifdef NDEBUG
    return;
#endif

    assert_not_zero(v_0);
    assert_not_zero(v_1);

    real dp = dot(normalize(v_0), normalize(v_1));
    real error = std::abs(dp);

    if (error < epsilon) {
        return;
    }

    stf::unreachable_with_message<"vectors are not orthogonal">();
}

}// namespace trc

#define VARIANT_CALL(_obj, _name, ...) \
    std::visit([&]<typename T>(T&& v) { return std::forward<T>(v)._name(__VA_ARGS__); }, _obj)

#define VARIANT_MEMBER(_obj, _name) \
    std::visit([&]<typename T>(T&& v) { return std::forward<T>(v)._name; }, _obj)
