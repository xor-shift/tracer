#pragma once

#include <tracer/material/concepts.hpp>

#include <tracer/material/simple_color.hpp>
#include <tracer/material/texture.hpp>

namespace trc {

using material = std::variant<materials::simple_color, materials::texture>;

template<typename Gen, concepts::material... Ts>
constexpr auto sample(std::variant<Ts...> const& material, intersection const& isection, Gen& gen) -> interaction {
    auto visitor = [&](concepts::material auto const& material) {
        return material.template sample<Gen>(isection, gen);
    };

    return std::visit(visitor, material);
}

template<concepts::material... Ts>
constexpr auto brdf(std::variant<Ts...> const& material, vec3 x, vec3 wi, vec3 wo) -> bool {
    auto visitor = [&](concepts::material auto const& material) {
        return material.brdf(x, wi, wo);
    };

    return std::visit(visitor, material);
}

template<concepts::material... Ts>
constexpr auto is_light(std::variant<Ts...> const& material) -> bool {
    auto visitor = [](concepts::material auto const& material) {
        return material.is_light();
    };

    return std::visit(visitor, material);
}

}// namespace trc
