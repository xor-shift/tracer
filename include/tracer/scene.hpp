#pragma once

#include <tracer/common.hpp>
#include <tracer/material/materials.hpp>
#include <tracer/shape/shapes.hpp>

namespace trc {

struct scene {
    constexpr scene() {}

    template<typename Fn>
    constexpr auto for_each_shape(Fn&& fn) const {
        for (bound_shape const& s: m_bound_shapes) {
            std::visit(std::forward<Fn>(fn), s);
        }

        for (unbound_shape const& s: m_unbound_shapes) {
            std::visit(std::forward<Fn>(fn), s);
        }

        if (m_bvh) {
            std::invoke(std::forward<Fn>(fn), *m_bvh);
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

    constexpr auto add_material(material mat) -> u32 {
        m_materials.emplace_back(std::move(mat));
        return static_cast<u32>(m_materials.size() - 1);
    }

    constexpr auto material(u32 index) const -> material const& {
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
        ray test_ray(a + dir * epsilon, dir);

        intersection isection;
        if (auto isection_res = intersect(test_ray); !isection_res) {
            return true;
        } else {
            isection = *isection_res;
        }

        auto t = isection.t * (1 + epsilon) >= dist;

        if (!t) {
            std::ignore = std::ignore;
        }

        return t;
    }

    constexpr void append_shape(bound_shape shape, usize split_threshold = 8, usize split_depth = 12) {
        m_bound_shapes.emplace_back(std::move(shape));

        append_if_over_threshold(split_threshold, split_depth);
    }

    constexpr void append_shape(unbound_shape shape) {
        m_unbound_shapes.emplace_back(std::move(shape));
    }

    constexpr void append_shapes(std::vector<bound_shape> shapes, usize split_threshold = 8, usize split_depth = 12) {
        m_bound_shapes.reserve(m_bound_shapes.size() + shapes.size());
        std::copy(shapes.begin(), shapes.end(), std::back_inserter(m_bound_shapes));

        append_if_over_threshold(split_threshold, split_depth);
    }

    constexpr void append_shapes(std::vector<unbound_shape> shapes) {
        std::copy(shapes.begin(), shapes.end(), std::back_inserter(m_unbound_shapes));
    }

    template<typename BVHType>
    void reconstruct_bvh(usize split_depth = 12) {
        if (m_bvh != nullptr) {
            std::vector<bound_shape> existing_shapes = m_bvh->deconstruct_tree();
            m_bound_shapes.reserve(m_bound_shapes.size() + existing_shapes.size());
            std::copy(existing_shapes.begin(), existing_shapes.end(), std::back_inserter(m_bound_shapes));
        }

        m_bvh = std::make_shared<BVHType>();
        m_bvh->construct_tree(std::move(m_bound_shapes), split_depth);
    }

    std::vector<trc::material> m_materials;

    std::vector<bound_shape> m_bound_shapes{};
    std::shared_ptr<dyn_shape<bound_shape>> m_bvh = nullptr;

    std::vector<unbound_shape> m_unbound_shapes{};

private:
    constexpr void append_if_over_threshold(usize split_threshold, usize split_depth) {
        if (m_bvh == nullptr) [[unlikely]] {
            // throw?
            return;
        }

        if (m_bound_shapes.size() >= split_threshold) {
            m_bvh->append(std::move(m_bound_shapes), split_depth);
        }
    }
};

}// namespace trc
