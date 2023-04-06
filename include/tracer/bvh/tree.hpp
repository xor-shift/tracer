#pragma once

#include <tracer/shape/shapes.hpp>

namespace trc {

namespace detail {

// this structure does not manage the child nodes on its own
struct bvh_node {
    constexpr bvh_node(std::vector<bound_shape> shapes, bvh_node* parent = nullptr, usize depth = 0)
        : m_parent(parent)
        , m_depth(depth)
        , m_shapes(std::move(shapes)) {
        compute_bounds();
    }

    /// Call only while destructing a bunch of nodes.
    constexpr auto extract_children() -> std::vector<bvh_node*> {
        return std::move(m_children);
    }

    constexpr auto split(usize depth = 1) -> bool {
        if (depth == 0) {
            return false;
        }

        std::vector<bound_shape> lhs_shapes{};
        std::vector<bound_shape> rhs_shapes{};

        vec3 side_lengths = m_bounds.second - m_bounds.first;
        usize longest_side = side_lengths[0] > side_lengths[1] ? side_lengths[0] > side_lengths[2] ? 0 : 2 : side_lengths[1] > side_lengths[2] ? 1
                                                                                                                                               : 2;
        real threshold = (m_bounds.first + side_lengths / 2)[longest_side];

        for (bound_shape const& shape: m_shapes) {
            vec3 shape_center = VARIANT_CALL(shape, center);
            (shape_center[longest_side] > threshold ? rhs_shapes : lhs_shapes).emplace_back(shape);
        }

        if (lhs_shapes.empty() || rhs_shapes.empty()) {
            return false;
        }

        m_children.emplace_back(new bvh_node(std::move(lhs_shapes)));
        m_children.emplace_back(new bvh_node(std::move(rhs_shapes)));

        if (depth != 0) {
            for (auto& child: m_children) {
                child->split(depth - 1);
            }
        }

        m_shapes.clear();

        return true;
    }

    constexpr auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> {
        if (!check_bounds_intersection(ray, m_bounds)) {
            return std::nullopt;
        }

        std::optional<intersection> best_isection = std::nullopt;

        auto iterate = [&best_isection, &best_t](std::optional<intersection> isection) {
            if (!isection)
                return;

            if (best_t > isection->t) {
                best_t = isection->t;
                best_isection = isection;
            }
        };

        if (m_shapes.empty()) {
            for (auto& child: m_children) {
                iterate(child->intersect(ray, best_t));
            }
        } else {
            for (auto const& shape: m_shapes) {
                iterate(VARIANT_CALL(shape, intersect, ray, best_t));
            }
        }

        return best_isection;
    }

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection> {
        pixel_statistics stats {};
        return intersect(ray, stats, best_t);
    }

    constexpr auto intersects(ray const& ray) const -> bool { return !!intersect(ray); }

    constexpr auto children() const -> std::span<const bvh_node* const> { return {m_children.data(), m_children.size()}; }

    constexpr auto parent() const -> bvh_node* { return m_parent; }

private:
    // consistency note: either m_shapes xor m_children are be allowed to be !empty() at a time

    bvh_node* m_parent = nullptr;// non-owning, memoization
    usize m_depth = 0;           // memoization

    std::vector<bound_shape> m_shapes{};

    vec3 m_center{0, 0, 0};
    std::pair<vec3, vec3> m_bounds{{0, 0, 0}, {0, 0, 0}};

    std::vector<bvh_node*> m_children{};

    constexpr void compute_bounds() {
        m_bounds.first = vec3(std::numeric_limits<real>::infinity());
        m_bounds.second = vec3(-std::numeric_limits<real>::infinity());

        auto iterate = [this](std::pair<vec3, vec3> bounds) {
            auto [min_bound, max_bound] = bounds;

            m_bounds.first = min(m_bounds.first, min_bound);
            m_bounds.first = min(m_bounds.first, max_bound);
            m_bounds.second = max(m_bounds.second, min_bound);
            m_bounds.second = max(m_bounds.second, max_bound);
        };

        if (m_shapes.empty()) {
            for (auto* child: m_children) {
                child->compute_bounds();
                iterate(child->m_bounds);
            }
        } else {
            m_center = {0, 0, 0};

            for (auto const& shape: m_shapes) {
                iterate(VARIANT_CALL(shape, bounds));
                m_center = m_center + VARIANT_CALL(shape, center);
            }

            m_center = m_center / static_cast<real>(m_shapes.size());
        }

        m_bounds.first = m_bounds.first * (1 + epsilon);
        m_bounds.second = m_bounds.second * (1 + epsilon);
    }
};

}// namespace detail

struct bvh_tree final : dyn_shape {
    constexpr bvh_tree(std::vector<bound_shape> shapes)
        : m_root(new detail::bvh_node(std::move(shapes))) {
        m_root->split(10);
    }

    constexpr ~bvh_tree() final override {
        std::vector<detail::bvh_node*> destruct_list{m_root};
        m_root = nullptr;

        for (usize i = 0;;) {
            bool did_a_thing = false;

            for (usize upto = destruct_list.size(); i < upto; i++) {
                detail::bvh_node*& cur = destruct_list[i];
                std::vector<detail::bvh_node*> cur_children = cur->extract_children();

                did_a_thing |= !cur_children.empty();

                std::copy(cur_children.begin(), cur_children.end(), std::back_inserter(destruct_list));
            }

            if (!did_a_thing)
                break;
        }

        for (auto*& node: destruct_list) {
            //node->detail::bvh_node::~bvh_node();
            delete node;
            node = nullptr;
        }
    }

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection> final override {
        return m_root->intersect(ray, best_t);
    }

    constexpr auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> final override {
        return m_root->intersect(ray, stats, best_t);
    }

    constexpr auto intersects(ray const& ray) const -> bool final override { return m_root->intersects(ray); }

private:
    detail::bvh_node* m_root = nullptr;
};

}// namespace trc
