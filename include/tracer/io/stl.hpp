#pragma once

#include <tracer/common.hpp>

#include <stuff/bit.hpp>
#include <stuff/core.hpp>
#include <stuff/expected.hpp>

#include <span>

namespace trc::io::stl {

struct triangle {
    stf::blas::vector<float, 3> normal;
    std::array<stf::blas::vector<float, 3>, 3> vertices;
};

template<typename It>
struct binary_stream {
    constexpr binary_stream(It it, It end)
        : m_it(it)
        , m_end(end) {
        discard_bytes(80);
        discard_bytes(4);
    }

    constexpr auto next() -> std::optional<triangle> {
        triangle ret{};

        if (
          !read_primitives<float>({ret.normal.data(), 3}) ||
          !read_primitives<float>({ret.vertices[0].data(), 3}) ||
          !read_primitives<float>({ret.vertices[1].data(), 3}) ||
          !read_primitives<float>({ret.vertices[2].data(), 3})) {
            return std::nullopt;
        }

        if (u16 attrib_size = TRYX(read_primitive<u16>()); !discard_bytes(attrib_size)) {
            return std::nullopt;
        }

        return ret;
    }

private:
    It m_it;
    It m_end;
    constexpr auto discard_bytes(usize n) -> bool {
        for (usize i = 0; i < n; i++) {
            if (m_it == m_end) {
                return false;
            }
            m_it++;
        }

        return true;
    }

    template<typename OIt>
    constexpr auto read_bytes(OIt out, usize n) {
        for (usize i = 0; i < n; i++) {
            if (m_it == m_end) {
                return false;
            }

            *out++ = *m_it++;
        }

        return true;
    }

    template<size_t Extent = std::dynamic_extent>
    constexpr auto read_bytes(std::span<u8, Extent> out) -> bool {
        return read_bytes(out.begin(), out.size());
    }

    template<typename T>
    constexpr auto read_primitive() -> std::optional<T> {
        std::array<u8, sizeof(T)> temp{};
        if (!read_bytes({temp})) {
            return std::nullopt;
        }

        return stf::bit::convert_endian(std::bit_cast<T>(temp), std::endian::little, std::endian::native);
    }

    template<typename T, size_t Extent = std::dynamic_extent>
    constexpr auto read_primitives(std::span<T, Extent> out) -> bool {
        for (T& elem: out) {
            std::optional<T> temp = read_primitive<T>();
            if (!temp) {
                return false;
            }
            elem = *temp;
        }

        return true;
    }
};

}// namespace trc::io::stl
