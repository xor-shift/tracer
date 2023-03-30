#pragma once

#include <tracer/intersection.hpp>
#include <tracer/texture.hpp>

#include <concepts>

namespace trc {

namespace concepts {

template<typename T>
concept material =//
  requires(T const& material, intersection const& isection) {
      //{ material.sample(isection) } -> std::convertible_to<material_sample>;
      true;
  };

}

using albedo_source = std::variant<color, texture>;

constexpr auto sample_albedo_source(albedo_source const& source, vec2 uv) -> color {
    stf::multi_visitor visitor{
      [](color c) { return c; },
      [uv](texture const& tex) { return tex.sample(uv); },
    };

    return std::visit(visitor, source);
}

struct material_base {
    constexpr material_base(albedo_source albedo, albedo_source emission = color(0)) noexcept
        : m_albedo(albedo)
        , m_emission(emission) {}

    constexpr auto is_light() const -> bool {
        stf::multi_visitor visitor{
          [](color c) { return vec3(0) != c; },
          [](texture const& tex) { return !tex.empty(); },
        };

        return std::visit(visitor, m_emission);
    }

protected:
    albedo_source m_albedo;
    albedo_source m_emission{vec3(0)};

    constexpr auto albedo_at(vec2 uv) const -> color {
        return sample_albedo_source(m_albedo, uv);
    }

    constexpr auto le_at(vec2 uv) const -> color {
        return sample_albedo_source(m_emission, uv);
    }
};

}
