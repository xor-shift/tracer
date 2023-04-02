#pragma once

#include <stuff/qoi.hpp>
#include <tracer/common.hpp>

namespace trc {

enum class wrapping_mode {
    extend = 0,
    clamp = extend,
    repeat,
};

enum class scaling_method {
    nearest = 0,
    bilinear,
    bicubic,
};

struct texture {
    constexpr texture() = default;

    constexpr texture(std::string_view filename, wrapping_mode wrapping_mode, scaling_method scaling_method)
        : texture() {
        from_file(filename, wrapping_mode, scaling_method);
    }

    template<typename Allocator>
    constexpr texture(stf::qoi::image<Allocator>&& image, wrapping_mode wrapping_mode, scaling_method scaling_method)
        : texture() {
        create(std::move(image), wrapping_mode, scaling_method);
    }

    template<typename Allocator>
    constexpr void create(stf::qoi::image<Allocator>&& image, wrapping_mode wrapping_mode, scaling_method scaling_method);

    auto from_file(std::string_view filename, wrapping_mode wrapping_mode, scaling_method scaling_method) -> stf::expected<void, std::string_view>;

    constexpr auto empty() const -> bool { return m_image.empty(); }

    constexpr auto sample(vec2 uv) const -> color;

private:
    stf::qoi::image<> m_image{};
    wrapping_mode m_wrapping_mode = wrapping_mode::clamp;
    scaling_method m_scaling_method = scaling_method::nearest;

    // cache
    real m_width = 0;
    real m_height = 0;

    constexpr auto sample_nearest(vec2 xy) const -> color;
};

}

#include <tracer/detail/texture.ipp>
