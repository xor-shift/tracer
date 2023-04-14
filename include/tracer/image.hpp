#pragma once

#include <tracer/common.hpp>

#include <stuff/qoi.hpp>

#ifdef TRACER_USING_SFML
#include <SFML/Graphics/Image.hpp>
#endif

namespace trc {

constexpr auto srgb_oetf(color linear) -> color {
    real k = 0.0031308;
    auto cutoff = less_than(swizzle<"rgb">(linear), vec3(k));

    vec3 higher = vec3(1.055) * pow(swizzle<"rgb">(linear), vec3(1. / 2.4)) - vec3(.055);
    vec3 lower = swizzle<"rgb">(linear) * vec3(12.92);

    return mix(higher, lower, cutoff);
}

constexpr auto srgb_eotf(color srgb) -> color {
    auto cutoff = less_than(swizzle<"rgb">(srgb), vec3(0.04045));
    vec3 higher = pow((swizzle<"rgb">(srgb) + vec3(0.055)) / vec3(1.055), vec3(2.4));
    vec3 lower = swizzle<"rgb">(srgb) / vec3(12.92);

    return mix(higher, lower, cutoff);
}

struct image_like {
    virtual constexpr ~image_like() = default;

    virtual constexpr auto width() const -> usize = 0;
    virtual constexpr auto height() const -> usize = 0;

    virtual constexpr void set(usize x, usize y, color c) = 0;
    virtual constexpr auto get(usize x, usize y) const -> color = 0;
};

/// Image adapter for images that contain sRGB data.\n
/// As with all adapters, this adapter should be used as a temporary and not stored anywhere.
template<typename ImageLike>
struct image_adapter_srgb : image_like {
    constexpr image_adapter_srgb(ImageLike& image)
        : m_image(image) {}

    virtual constexpr auto width() const -> usize override { return m_image.width(); }
    virtual constexpr auto height() const -> usize override { return m_image.height(); }

    /// Converts <code>c</code> from linear RGB to sRGB before writing to the adapted image
    virtual constexpr void set(usize x, usize y, color c) override { return m_image.set(x, y, srgb_oetf(c)); }

    /// Converts the color retrieved from the adapted image from sRGB to linear RGB
    virtual constexpr auto get(usize x, usize y) const -> color override { return srgb_eotf(m_image.get(x, y)); }

private:
    ImageLike& m_image;
};

/// Writes pixels to multiple images\n
/// As with all adapters, this adapter should be used as a temporary and not stored anywhere.
template<typename... Images>
struct image_adapter_tee : image_like {
    constexpr image_adapter_tee(Images&... images)
        : m_images({std::addressof(static_cast<image_like&>(images))...}) {
        invalidate_dimensions();
    }

    constexpr void invalidate_dimensions() {
        m_width = std::numeric_limits<usize>::max();
        m_height = std::numeric_limits<usize>::max();

        for (image_like* image: m_images) {
            m_width = std::min(m_width, image->width());
            m_height = std::min(m_height, image->height());
        }
    }

    virtual constexpr auto width() const -> usize override { return m_width; }
    virtual constexpr auto height() const -> usize override { return m_height; }

    virtual constexpr void set(usize x, usize y, color c) override {
        for (image_like* image: m_images) {
            image->set(x, y, c);
        }
    }

    virtual constexpr auto get(usize x, usize y) const -> color override { return {}; }

private:
    std::vector<image_like*> m_images{};

    usize m_width = 0;
    usize m_height = 0;
};

#if TRACER_USING_SFML

/// Adapts an SFML <code>sf::Image</code>.\n
/// The color mapping is from [0, 1] to [0, 255], rounding used is to the nearest integer. NaNs are treated as 0 while reading.\n
/// sRGB might be assumed by some SFML functions, this adapter won't take care of that. See <code>image_adapter_srgb</code>\n
/// As with all adapters, this adapter should be used as a temporary and not stored anywhere.
struct image_adapter_sfml : image_like {
    constexpr image_adapter_sfml(sf::Image& image)
        : m_image(image) {}


    virtual auto width() const -> usize override { return m_image.getSize().x; }
    virtual auto height() const -> usize override { return m_image.getSize().y; }

    virtual void set(usize x, usize y, color c) override {
        c = round(clamp(c, 0, 1) * 255);

        sf::Color sf_color{
          std::isnan(c[0]) ? u8(255) : static_cast<u8>(c[0]),
          std::isnan(c[1]) ? u8(255) : static_cast<u8>(c[1]),
          std::isnan(c[2]) ? u8(255) : static_cast<u8>(c[2]),
          255,
        };

        m_image.setPixel(sf::Vector2u(x, y), sf_color);
    }

    virtual auto get(usize x, usize y) const -> color override {
        sf::Color sf_color = m_image.getPixel(sf::Vector2u(x, y));
        color c{sf_color.r, sf_color.g, sf_color.b};
        return c / 255;
    }

private:
    sf::Image& m_image;
};

#endif

/// Adapts an SFML <code>sf::Image</code>.\n
/// The color mapping is from [0, 1] to [0, 255], rounding used is to the nearest integer. NaNs are treated as 0 while reading.\n
/// sRGB mapping will be done according to the adapted qoi image.\n
/// As with all adapters, this adapter should be used as a temporary and not stored anywhere.
template<typename Allocator = std::allocator<stf::qoi::color>>
struct image_adapter_qoi : image_like {
    constexpr image_adapter_qoi(stf::qoi::image<Allocator>& image)
        : m_image(image) {}


    virtual constexpr auto width() const -> usize override { return m_image.width(); }
    virtual constexpr auto height() const -> usize override { return m_image.height(); }

    virtual constexpr void set(usize x, usize y, color c) override {
        if (m_image.get_color_space() == stf::qoi::color_space::srgb_linear_alpha) {
            c = srgb_oetf(c);
        }

        c = round(clamp(c, 0, 1) * 255);

        stf::qoi::color qoi_color{
          std::isnan(c[0]) ? u8(255) : static_cast<u8>(c[0]),
          std::isnan(c[1]) ? u8(255) : static_cast<u8>(c[1]),
          std::isnan(c[2]) ? u8(255) : static_cast<u8>(c[2]),
          255,
        };

        m_image.at(x, y) = qoi_color;
    }

    virtual constexpr auto get(usize x, usize y) const -> color override {
        stf::qoi::color qoi_color = m_image.at(x, y);

        color c{qoi_color.r, qoi_color.g, qoi_color.b};
        c = c / 255;

        if (m_image.get_color_space() == stf::qoi::color_space::srgb_linear_alpha) {
            c = srgb_eotf(c);
        }

        return c;
    }

private:
    stf::qoi::image<Allocator>& m_image;
};

template<typename Color, typename Allocator = std::allocator<Color>>
struct basic_image : public image_like {
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

    constexpr auto width() const -> usize override { return m_width; }
    constexpr auto height() const -> usize override { return m_height; }
    constexpr auto size() const -> u64 { return static_cast<u64>(width()) * static_cast<u64>(height()); }

    constexpr auto data() -> color_type* { return m_data; }
    constexpr auto data() const -> const color_type* { return m_data; }
    constexpr auto pixels() -> std::span<color_type> { return {m_data, m_width * m_height}; }
    constexpr auto pixels() const -> std::span<const color_type> { return {m_data, m_width * m_height}; }

    constexpr auto at(usize x, usize y) -> color_type& { return m_data[y * m_width + x]; }
    constexpr auto at(usize x, usize y) const -> color_type { return m_data[y * m_width + x]; }

    constexpr void set(usize x, usize y, color c) override { at(x, y) = c; }
    constexpr auto get(usize x, usize y) const -> color override { return at(x, y); }

private:
    Allocator m_allocator;
    color_type* m_data = nullptr;
    usize m_width = 0;
    usize m_height = 0;
};

template<typename Color>
struct basic_image_view : image_like {
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

    constexpr auto width() const -> usize override { return m_view_dims.first; }
    constexpr auto height() const -> usize override { return m_view_dims.second; }
    constexpr auto size() const -> u64 { return static_cast<u64>(width()) * static_cast<u64>(height()); }

    constexpr auto at(usize x, usize y) -> color_type& { return m_data[index(x, y)]; }
    constexpr auto at(usize x, usize y) const -> color_type { return m_data[index(x, y)]; }

    constexpr void set(usize x, usize y, color c) override { at(x, y) = c; }
    constexpr auto get(usize x, usize y) const -> color override { return at(x, y); }

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
