#pragma once

#include <tracer/common.hpp>

namespace trc {

template<typename Color, typename Allocator = std::allocator<Color>>
struct basic_image {
    using color_type = Color;

    constexpr basic_image(Allocator const& allocator = {}) noexcept(noexcept(Allocator(allocator)))
        : m_allocator(allocator) {}

    constexpr basic_image(usize width, usize height, Allocator const& allocator = {}) {
        create(width, height);
    }

    constexpr basic_image(basic_image const& other)
        : basic_image() {
        *this = other;
    }

    constexpr basic_image(basic_image&& other)
        : basic_image() {
        *this = std::move(other);
    }

    constexpr ~basic_image() { destroy(); }

    constexpr auto operator=(basic_image const& other) -> basic_image& {
        m_allocator = other.m_allocator;
        m_data = m_allocator.allocate(other.m_width * other.m_height);
        m_width = other.m_width;
        m_height = other.m_height;

        std::copy_n(other.m_data, size(), m_data);

        return *this;
    }

    constexpr auto operator=(basic_image&& other) -> basic_image& {
        m_allocator = other.m_allocator;
        m_data = other.m_data;
        m_width = other.m_width;
        m_height = other.m_height;

        other.m_data = nullptr;

        return *this;
    }

    constexpr auto empty() const -> bool { return m_data == nullptr || m_width == 0 || m_height == 0; }

    constexpr void destroy() noexcept {
        if (!empty()) {
            m_allocator.deallocate(m_data, m_width * m_height);
            m_data = nullptr;
            m_width = 0;
            m_height = 0;
        }
    }

    constexpr void create(usize width, usize height) noexcept(noexcept(m_allocator.allocate(0))) {
        destroy();

        m_width = width;
        m_height = height;

        if (width == 0 || height == 0) {
            return;
        }

        m_data = m_allocator.allocate(width * height * 4);
    }

    constexpr void fill(color_type c) { std::fill(m_data, m_data + m_width * m_height, c); }

    constexpr auto width() const -> usize { return m_width; }
    constexpr auto height() const -> usize { return m_height; }
    constexpr auto size() const -> u64 { return static_cast<u64>(width()) * static_cast<u64>(height()); }

    constexpr auto data() -> color_type* { return m_data; }
    constexpr auto data() const -> const color_type* { return m_data; }
    constexpr auto pixels() -> std::span<color_type> { return {m_data, m_width * m_height}; }
    constexpr auto pixels() const -> std::span<const color_type> { return {m_data, m_width * m_height}; }

    constexpr auto at(usize x, usize y) -> color_type& { return m_data[y * m_width + x]; }
    constexpr auto at(usize x, usize y) const -> color_type { return m_data[y * m_width + x]; }

private:
    Allocator m_allocator;
    color_type* m_data = nullptr;
    usize m_width = 0;
    usize m_height = 0;
};

template<typename Color>
struct basic_image_view {
    using color_type = Color;

    constexpr basic_image_view() = default;

    constexpr basic_image_view(basic_image<Color>& other, usize x = 0, usize y = 0, usize extent_x = -1, usize extent_y = -1) noexcept
        : m_data(other.data())
        , m_data_dims({other.width(), other.height()}) {
        x = std::min(x, other.width() - 1);
        y = std::min(y, other.height() - 1);

        m_offset = {x, y};

        usize upto_x = std::min(extent_x + x, other.width());
        usize upto_y = std::min(extent_y + y, other.height());

        m_view_dims.first = upto_x - x;
        m_view_dims.second = upto_y - y;
    }

    constexpr basic_image_view(basic_image_view<Color> other, usize x, usize y, usize extent_x = -1, usize extent_y = -1) noexcept
        : m_data(other.m_data)
        , m_data_dims(other.m_data_dims) {
        x = std::min(x + other.m_offset.first, other.width() - 1);
        y = std::min(y + other.m_offset.second, other.height() - 1);

        m_offset = {x, y};

        usize pretend_original_width = other.width() + other.m_offset.first;
        usize pretend_original_height = other.height() + other.m_offset.second;

        usize upto_x = std::min(extent_x + x, pretend_original_width);
        usize upto_y = std::min(extent_y + y, pretend_original_height);

        m_view_dims.first = upto_x - x;
        m_view_dims.second = upto_y - y;
    }

    constexpr auto empty() const -> bool {
        return m_data == nullptr || width() == 0 || height() == 0;
    }

    //constexpr void fill(color_type c) { std::fill(m_data, m_data + m_width * m_height, c); }

    constexpr auto width() const -> usize { return m_view_dims.first; }
    constexpr auto height() const -> usize { return m_view_dims.second; }
    constexpr auto size() const -> u64 { return static_cast<u64>(width()) * static_cast<u64>(height()); }

    constexpr auto at(usize x, usize y) -> color_type& { return m_data[index(x, y)]; }
    constexpr auto at(usize x, usize y) const -> color_type { return m_data[index(x, y)]; }

private:
    color_type* m_data = nullptr;
    std::pair<usize, usize> m_data_dims;

    std::pair<usize, usize> m_offset;
    std::pair<usize, usize> m_view_dims;

    constexpr auto index(usize x, usize y) const -> usize {
        usize real_x = x + m_offset.first;
        usize real_y = y + m_offset.second;
        return real_x + real_y * m_data_dims.first;
    }
};

using image = basic_image<color, std::allocator<color>>;
using image_view = basic_image_view<color>;

}// namespace trc
