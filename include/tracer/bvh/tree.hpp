#pragma once

#include <tracer/shape/shapes.hpp>

namespace trc {

namespace detail {

// this structure does not manage the child nodes on its own
struct bvh_node {
    constexpr bvh_node(std::vector<bound_shape> shapes)
        : m_shapes(std::move(shapes)) {
        compute_bounds();
    }

    /// Call only while destructing a bunch of nodes.
    constexpr auto extract_children() -> std::vector<bvh_node*> {
        return std::move(m_children);
    }

    constexpr void split(size_t depth = 1) {
        if (depth == 0) {
            return;
        }

        std::vector<bound_shape> shapes(std::move(m_shapes));

        std::vector<bound_shape> lhs_shapes {shapes.begin(), shapes.begin() + shapes.size() / 2 + 1};
        std::vector<bound_shape> rhs_shapes {shapes.begin() + shapes.size() / 2 + 1, shapes.end()};

        m_children.emplace_back(new bvh_node(std::move(lhs_shapes)));
        m_children.emplace_back(new bvh_node(std::move(rhs_shapes)));
    }

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection> {
        if (!check_bounds_intersection(ray, m_bounds)) {
            return std::nullopt;
        }

        std::optional<intersection> best_isection = std::nullopt;

        auto iterate = [this, &best_isection, &best_t](std::optional<intersection> isection) {
            if (!isection)
                return;

            if (best_t > isection->t) {
                best_t = isection->t;
                best_isection = isection;
            }
        };

        if (m_shapes.empty()) {
            for (auto* child: m_children) {
                iterate(child->intersect(ray, best_t));
            }
        } else {
            for (auto const& shape: m_shapes) {
                iterate(VARIANT_CALL(shape, intersect, ray, best_t));
            }
        }

        return best_isection;
    }

    constexpr auto intersects(ray const& ray) const -> bool { return !!intersect(ray); }

private:
    // consistency note: either m_shapes xor m_children are be allowed to be !empty() at a time

    std::vector<bound_shape> m_shapes;

    std::pair<vec3, vec3> m_bounds;
    std::vector<bvh_node*> m_children;

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
            for (auto const& shape: m_shapes) {
                iterate(VARIANT_CALL(shape, bounds));
            }
        }

        m_bounds.first = m_bounds.first * (1 + epsilon);
        m_bounds.second = m_bounds.second * (1 + epsilon);
    }
};

}// namespace detail

struct bvh_tree : dyn_shape {
    constexpr bvh_tree(std::vector<bound_shape> shapes)
        : m_root(new detail::bvh_node(std::move(shapes))) {
        m_root->split();
    }

    constexpr ~bvh_tree() {
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

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection> {
        return m_root->intersect(ray, best_t);
    }

    constexpr auto intersects(ray const& ray) const -> bool { return m_root->intersects(ray); }

private:
    detail::bvh_node* m_root = nullptr;
};

}// namespace trc
