#pragma once

#include <tracer/common.hpp>
#include <tracer/material/materials.hpp>
#include <tracer/shape/shapes.hpp>

namespace trc {

struct scene {
    constexpr scene(std::vector<shape> const& shapes, std::vector<material> materials, std::vector<std::unique_ptr<dyn_shape>>&& dyn_shapes = {})
        : m_materials(std::move(materials))
        , m_dyn_shapes(std::move(dyn_shapes)) {
        for (shape s: shapes) {
            auto visitor = stf::multi_visitor{
              [this]<concepts::bound_shape T>(T&& s) { m_bound_shapes.emplace_back(std::move(s)); },
              [this]<concepts::unbound_shape T>(T&& s) { m_unbound_shapes.emplace_back(std::move(s)); },
            };
            std::visit(visitor, std::move(s));
        }
    }

    template<typename Fn>
    constexpr auto for_each_shape(Fn&& fn) const {
        for (bound_shape const& s: m_bound_shapes) {
            std::visit(std::forward<Fn>(fn), s);
        }

        for (unbound_shape const& s: m_unbound_shapes) {
            std::visit(std::forward<Fn>(fn), s);
        }

        for (auto const& s: m_dyn_shapes) {
            std::invoke(std::forward<Fn>(fn), *s);
        }
    }

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection> {
        std::optional<intersection> best_isection = std::nullopt;

        for_each_shape([&](concepts::shape auto const& shape) {
            std::optional<intersection> isection = shape.intersect(ray, best_t);
            if (!isection)
                return;

            if (best_t > isection->t) {
                best_t = isection->t;
                best_isection = isection;
            }
        });

        return best_isection;
    }

    constexpr auto material(usize index) const -> material const& {
        return m_materials[index];
    }

    template<typename Gen>
    constexpr auto pick_light(Gen& gen) const -> bound_shape {
        bound_shape ret;
        std::uniform_int_distribution<usize> dist(0, m_bound_shapes.size() - 1);

        do {
            ret = m_bound_shapes[dist(gen)];
        } while (!VARIANT_CALL(m_materials[VARIANT_CALL(ret, material_index)], is_light));

        return ret;
    }

    constexpr auto visibility_check(vec3 a, vec3 b) const -> bool {
        real dist = abs(b - a);
        vec3 dir = (b - a) / dist;
        ray test_ray(a + dir * 0.0001, dir);

        intersection isection;
        if (auto isection_res = intersect(test_ray); !isection_res) {
            return true;
        } else {
            isection = *isection_res;
        }

        auto t = isection.t + 0.01 >= dist;

        if (!t) {
            std::ignore = std::ignore;
        }

        return t;
    }

private:
    std::vector<trc::material> m_materials;

    std::vector<std::unique_ptr<dyn_shape>> m_dyn_shapes{};
    std::vector<bound_shape> m_bound_shapes{};
    std::vector<unbound_shape> m_unbound_shapes{};
};

}// namespace trc
