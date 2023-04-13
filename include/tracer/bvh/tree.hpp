#pragma once

#include <tracer/common.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>
#include <tracer/shape/box.hpp>

#include <stack>

namespace trc {

namespace detail {

template<typename T, typename Fn>
static constexpr auto compute_bounds(std::span<T> objects, Fn&& bound_getter) -> std::pair<vec3, vec3> {
    bounding_box box{};

    for (T const& object: objects) {
        box.bump(std::invoke(bound_getter, object));
    }

    box.extend_a_little();

    return box.bounds;
}

}// namespace detail

template<typename ShapeT>
struct generic_bvh_node {
    constexpr generic_bvh_node() = default;

    template<typename CenterFn, typename BoundsFn>
    constexpr void construct(std::span<const ShapeT> all_shapes, std::pair<usize, usize> shape_extents, usize depth, CenterFn&& center_fn, BoundsFn&& bounds_fn) {
        m_depth = depth;
        m_shape_extents = shape_extents;
        compute_bounds<CenterFn, BoundsFn>(all_shapes, std::forward<CenterFn>(center_fn), std::forward<BoundsFn>(bounds_fn));
    }

    template<typename CenterFn, typename BoundsFn>
    constexpr void compute_bounds(std::span<const ShapeT> all_shapes, CenterFn&& center_fn, BoundsFn&& bounds_fn) {
        std::span<const ShapeT> shapes_span = get_shapes_span(all_shapes);

        m_center = {0, 0, 0};
        m_bounds = ::trc::detail::compute_bounds(shapes_span, [&, this](ShapeT const& shape) {
            //m_center = m_center + VARIANT_CALL(shape, center);
            //return VARIANT_CALL(shape, bounds);

            m_center = m_center + std::invoke(std::forward<CenterFn>(center_fn), shape);
            return std::invoke(std::forward<BoundsFn>(bounds_fn), shape);
        });
        m_center = m_center / static_cast<real>(shapes_span.size());
    }

    template<typename CenterFn>
    constexpr auto partition_longest_axis(std::span<ShapeT> all_shapes, CenterFn&& center_fn) -> std::optional<usize> {
        if (!m_is_leaf) {
            return std::nullopt;
        }

        std::span<ShapeT> shapes_span = get_shapes_span(all_shapes);
        vec3 side_lengths = m_bounds.second - m_bounds.first;
        usize longest_side = side_lengths[0] > side_lengths[1] ? side_lengths[0] > side_lengths[2] ? 0 : 2 : side_lengths[1] > side_lengths[2] ? 1
                                                                                                                                               : 2;
        real threshold = (m_bounds.first + side_lengths / 2)[longest_side];

        auto midpoint = std::partition(shapes_span.begin(), shapes_span.end(), [&](auto const& shape) {
            //vec3 shape_center = VARIANT_CALL(shape, center);
            vec3 shape_center = std::invoke(std::forward<CenterFn>(center_fn), shape);
            return shape_center[longest_side] < threshold;
        });

        if (midpoint == shapes_span.begin() || midpoint == shapes_span.end()) {
            return std::nullopt;
        }

        return std::distance(shapes_span.begin(), midpoint);
    }

    template<typename CenterFn, typename BoundsFn>
    constexpr auto partition_sah(std::span<ShapeT> all_shapes, CenterFn&& center_fn, BoundsFn&& bounds_fn) -> std::optional<usize> {
        std::span<ShapeT> shapes_span = get_shapes_span(all_shapes);

        auto heuristic = [&](usize axis, real split_at) {
            usize count_lhs = 0;
            std::pair<vec3, vec3> bounds_lhs{vec3(std::numeric_limits<real>::infinity()), vec3(-std::numeric_limits<real>::infinity())};

            usize count_rhs = 0;
            std::pair<vec3, vec3> bounds_rhs{vec3(std::numeric_limits<real>::infinity()), vec3(-std::numeric_limits<real>::infinity())};

            auto iterate = [](std::pair<vec3, vec3>& iterate_on, std::pair<vec3, vec3> cur_bounds) {
                auto [min_bound, max_bound] = cur_bounds;

                iterate_on.first = min(iterate_on.first, min_bound);
                iterate_on.first = min(iterate_on.first, max_bound);
                iterate_on.second = max(iterate_on.second, min_bound);
                iterate_on.second = max(iterate_on.second, max_bound);
            };

            for (auto const& shape: shapes_span) {
                //vec3 shape_center = VARIANT_CALL(shape, center);
                //std::pair<vec3, vec3> shape_bounds = VARIANT_CALL(shape, bounds);

                vec3 shape_center = std::invoke(std::forward<CenterFn>(center_fn), shape);
                std::pair<vec3, vec3> shape_bounds = std::invoke(std::forward<BoundsFn>(bounds_fn), shape);

                if (shape_center[axis] < split_at) {
                    ++count_lhs;
                    iterate(bounds_lhs, shape_bounds);
                } else {
                    ++count_rhs;
                    iterate(bounds_rhs, shape_bounds);
                }
            }

            vec3 lhs_side_lengths = (bounds_lhs.second - bounds_lhs.first);
            vec3 rhs_side_lengths = (bounds_rhs.second - bounds_rhs.first);

            real sah = 0;

            sah += (lhs_side_lengths[0] * lhs_side_lengths[1] * lhs_side_lengths[2]) * static_cast<real>(count_lhs);
            sah += (rhs_side_lengths[0] * rhs_side_lengths[1] * rhs_side_lengths[2]) * static_cast<real>(count_rhs);

            return sah;
        };

        usize best_axis = 0;
        real best_param = 0;
        real best_sah = std::numeric_limits<real>::infinity();

        for (usize axis = 0; axis < 3; axis++) {
            const usize step_ct = 75;
            real granularity = (m_bounds.second - m_bounds.first)[axis] / static_cast<real>(step_ct);

            for (usize step = 0; step < step_ct; step++) {
                real param = granularity * static_cast<real>(step) + m_bounds.first[axis];

                real cur_heuristic = heuristic(axis, param);

                if (best_sah > cur_heuristic) {
                    best_axis = axis;
                    best_param = param;
                    best_sah = cur_heuristic;
                }
            }

            /*for (auto const& shape : shapes_span) {
                vec3 shape_center = VARIANT_CALL(shape, center);
                real cur_heuristic = heuristic(axis, shape_center[axis]);

                if (best_sah > cur_heuristic) {
                    best_axis = axis;
                    best_param = shape_center[axis];
                    best_sah = cur_heuristic;
                }
            }*/
        }

        auto midpoint = std::partition(shapes_span.begin(), shapes_span.end(), [&](auto const& shape) {
            //vec3 shape_center = VARIANT_CALL(shape, center);
            vec3 shape_center = std::invoke(std::forward<CenterFn>(center_fn), shape);
            return shape_center[best_axis] < best_param;
        });

        if (midpoint == shapes_span.begin() || midpoint == shapes_span.end()) {
            return std::nullopt;
        }

        return std::distance(shapes_span.begin(), midpoint);
    }

    template<typename CenterFn, typename BoundsFn>
    constexpr auto split(std::span<ShapeT> all_shapes, generic_bvh_node& left_candidate, generic_bvh_node& right_candidate, CenterFn&& center_fn, BoundsFn&& bounds_fn) -> bool {
        auto res = partition_sah<CenterFn, BoundsFn>(all_shapes, std::forward<CenterFn>(center_fn), std::forward<BoundsFn>(bounds_fn));
        if (!res) {
            return false;
        }

        m_is_leaf = false;

        left_candidate.construct<CenterFn, BoundsFn>(all_shapes, {m_shape_extents.first, m_shape_extents.first + *res}, m_depth + 1, std::forward<CenterFn>(center_fn), std::forward<BoundsFn>(bounds_fn));
        right_candidate.construct<CenterFn, BoundsFn>(all_shapes, {m_shape_extents.first + *res, m_shape_extents.second}, m_depth + 1, std::forward<CenterFn>(center_fn), std::forward<BoundsFn>(bounds_fn));

        return true;
    }

    constexpr auto bound_check(ray const& ray) const -> bool {
        return check_bounds_intersection(ray, m_bounds);
    }

    template<typename IntersectFn>
    constexpr auto intersect(std::span<const ShapeT> all_shapes, ray const& ray, pixel_statistics& stats, real best_t, IntersectFn&& intersect_fn) const -> std::optional<intersection> {
        if (!bound_check(ray)) {
            return std::nullopt;
        }

        std::span<const ShapeT> shapes_span = get_shapes_span(all_shapes);

        std::optional<intersection> best_isection = std::nullopt;

        auto iterate = [&best_isection, &best_t](std::optional<intersection> isection) {
            if (!isection)
                return;

            if (best_t > isection->t) {
                best_t = isection->t;
                best_isection = isection;
            }
        };

        for (auto const& shape: shapes_span) {
            //iterate(VARIANT_CALL(shape, intersect, ray, best_t));
            iterate(std::invoke(intersect_fn, shape, ray, best_t));
        }

        return best_isection;
    }

    constexpr auto intersects(std::span<const ShapeT> all_shapes, ray const& ray) const -> bool {
        return !!intersect(all_shapes, ray);
    }

    constexpr auto is_leaf() const -> bool { return m_is_leaf; }

    constexpr auto get_shapes_span(std::span<const ShapeT> all_shapes) const -> std::span<const ShapeT> {
        return all_shapes.subspan(m_shape_extents.first, m_shape_extents.second - m_shape_extents.first);
    }

    constexpr auto get_shapes_span(std::span<ShapeT> all_shapes) const -> std::span<ShapeT> {
        return all_shapes.subspan(m_shape_extents.first, m_shape_extents.second - m_shape_extents.first);
    }

    constexpr auto empty() const -> bool { return m_shape_extents.first == m_shape_extents.second; }

private:
    usize m_depth;
    std::pair<usize, usize> m_shape_extents;

    vec3 m_center{0, 0, 0};
    std::pair<vec3, vec3> m_bounds;
    bool m_is_leaf = true;
};

template<typename ShapeT>
struct generic_bvh {
    using node_handle = usize;
    using node_type = generic_bvh_node<ShapeT>;
    inline static constexpr node_handle bad_handle = -1;

    static constexpr auto node_is_root(node_handle handle) -> bool { return handle == 0; }

    constexpr auto node_is_at_max_depth(node_handle handle) -> bool { return handle >= m_nodes.size() / 2; }

    static constexpr auto parent_node(node_handle handle) -> node_handle {
        return handle == 0 ? bad_handle : (handle - 1) / 2;
    }

    constexpr auto left_child(node_handle handle) const -> node_handle {
        node_handle ret = handle * 2 + 1;
        if (ret >= m_nodes.size()) {
            return bad_handle;
        }
        return ret;
    }

    constexpr auto right_child(node_handle handle) const -> node_handle {
        node_handle ret = handle * 2 + 2;
        if (ret >= m_nodes.size()) {
            return bad_handle;
        }
        return ret;
    }

    constexpr auto node_at_handle(node_handle handle) const noexcept -> node_type const& {
        return m_nodes[handle];
    }

    constexpr auto node_at_handle(node_handle handle) noexcept -> node_type& {
        node_type const& const_node = const_cast<const generic_bvh<ShapeT>*>(this)->node_at_handle(handle);
        return const_cast<node_type&>(const_node);
    }

    template<typename CenterFn, typename BoundsFn>
    constexpr void construct_tree(std::vector<ShapeT> shapes, usize depth, CenterFn&& center_fn, BoundsFn&& bounds_fn) {
        m_shapes = std::move(shapes);

        return construct_tree<CenterFn, BoundsFn>(depth, std::forward<CenterFn>(center_fn), std::forward<BoundsFn>(bounds_fn));
    }

    template<typename CenterFn, typename BoundsFn>
    constexpr void construct_tree(usize depth, CenterFn&& center_fn, BoundsFn&& bounds_fn) {
        m_depth = depth + 1;

        m_nodes.clear();
        m_nodes = std::vector<node_type>((1uz << m_depth) - 1);
        m_nodes[0].construct(m_shapes, std::pair{0uz, m_shapes.size()}, 0, std::forward<CenterFn>(center_fn), std::forward<BoundsFn>(bounds_fn));

        split_at<CenterFn, BoundsFn>(0, std::forward<CenterFn>(center_fn), std::forward<BoundsFn>(bounds_fn));

        while (last_layer_is_vacant()) {
            trim_last_layer();
        }

        return;
    }

    constexpr auto deconstruct_tree() -> std::vector<ShapeT> {
        m_nodes.clear();
        m_depth = 0;

        return std::move(m_shapes);
    }

    template<typename Fn>
    constexpr void traverse_candidates(ray const& ray, Fn&& fn) const {
        std::stack<node_handle> to_traverse{};
        to_traverse.push(0);

        while (!to_traverse.empty()) {
            node_handle cur_handle = to_traverse.top();
            to_traverse.pop();

            node_type const& cur_node = node_at_handle(cur_handle);

            if (!cur_node.bound_check(ray)) {
                continue;
            }

            if (cur_node.is_leaf()) {
                std::invoke(std::forward<Fn>(fn), cur_node);
                continue;
            }

            to_traverse.push(left_child(cur_handle));
            to_traverse.push(right_child(cur_handle));
        }
    }

    constexpr auto last_layer_is_vacant() const -> bool {
        for (usize i = m_nodes.size() / 2; i < m_nodes.size(); i++) {
            if (!m_nodes[i].empty()) {
                return false;
            }
        }
        return true;
    }

    constexpr void trim_last_layer() {
        --m_depth;
        m_nodes.resize(m_nodes.size() / 2 - 1);
        m_nodes.shrink_to_fit();
    }

protected:
    std::vector<ShapeT> m_shapes{};

private:
    usize m_depth = 0;
    std::vector<node_type> m_nodes{};

    template<typename CenterFn, typename BoundsFn>
    constexpr void split_at(node_handle handle, CenterFn&& center_fn, BoundsFn&& bounds_fn) {
        if (node_is_at_max_depth(handle)) {
            return;
        }

        node_type& cur_node = node_at_handle(handle);
        node_type& left_candidate = node_at_handle(left_child(handle));
        node_type& right_candidate = node_at_handle(right_child(handle));

        if (!cur_node.split(
              m_shapes, left_candidate, right_candidate,//
              std::forward<CenterFn>(center_fn),        //
              std::forward<BoundsFn>(bounds_fn)         //

              //[](auto const& shape) { return VARIANT_CALL(shape, center); },
              //[](auto const& shape) { return VARIANT_CALL(shape, bounds); }
              )) {
            return;
        }

        split_at<CenterFn, BoundsFn>(left_child(handle), std::forward<CenterFn>(center_fn), std::forward<BoundsFn>(bounds_fn));
        split_at<CenterFn, BoundsFn>(right_child(handle), std::forward<CenterFn>(center_fn), std::forward<BoundsFn>(bounds_fn));
    }
};

template<typename ShapeT>
struct binary_bvh final : generic_bvh<ShapeT>, dyn_shape<ShapeT> {
    using node_handle = usize;
    using node_type = generic_bvh_node<ShapeT>;
    inline static constexpr node_handle bad_handle = -1;

    constexpr auto intersect(ray const& ray, real best_t = infinity) const -> std::optional<intersection> final override {
        pixel_statistics stats{};
        return intersect(ray, stats, best_t);
    }

    constexpr auto intersect(ray const& ray, pixel_statistics& stats, real best_t = infinity) const -> std::optional<intersection> final override {
        std::optional<intersection> best_isection = std::nullopt;

        auto iterate = [&best_isection, &best_t](std::optional<intersection> isection) {
            if (!isection)
                return;

            if (best_t > isection->t) {
                best_t = isection->t;
                best_isection = isection;
            }
        };

        generic_bvh<ShapeT>::traverse_candidates(ray, [&](node_type const& node) {
            iterate(node.intersect(this->m_shapes, ray, stats, best_t, [](auto const& shape, ::trc::ray const& ray, real best_t) { return VARIANT_CALL(shape, intersect, ray, best_t); }));
        });

        return best_isection;
    }

    constexpr auto intersects(ray const& ray) const -> bool final override {
        return !!intersect(ray);
    }

    constexpr void construct_tree(std::vector<ShapeT> shapes, usize depth) final override {
        return generic_bvh<ShapeT>::construct_tree(
          shapes, depth,                                                //
          [](auto const& shape) { return VARIANT_CALL(shape, center); },//
          [](auto const& shape) { return VARIANT_CALL(shape, bounds); });
    }

    constexpr auto deconstruct_tree() -> std::vector<ShapeT> final override {
        return generic_bvh<ShapeT>::deconstruct_tree();
    }
};

}// namespace trc
