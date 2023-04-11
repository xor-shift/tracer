#pragma once

#include <stuff/ranvec.hpp>
#include <tracer/intersection.hpp>
#include <tracer/ray.hpp>

#include <optional>

namespace trc::shapes {

struct point {
    constexpr auto intersect(ray const&) const -> std::optional<intersection> { return std::nullopt; }

    constexpr auto intersects(ray const&) const -> bool { return false; }

    constexpr auto bounds() const -> std::pair<vec3, vec3> { return {m_center, m_center}; }

    constexpr auto center() const -> vec3 { return m_center; }

    template<typename Gen>
    constexpr auto sample_surface(Gen&) const -> vec3 { return m_center; }

    constexpr auto material_index() const -> u32 { return m_mat_idx; }

    constexpr void set_material(u32 idx) { m_mat_idx = idx; }

private:
    vec3 m_center;
    u32 m_mat_idx;
};

}// namespace trc::shapes
