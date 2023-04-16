#pragma once

#include <stuff/expected.hpp>

#include <tracer/common.hpp>

#include <range/v3/range.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/getlines.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/take_while.hpp>
#include <range/v3/view/transform.hpp>

#include <spdlog/spdlog.h>

#include <charconv>

namespace trc::io::ply {

enum class primitive_type {
    s8,
    u8,
    s16,
    u16,
    s32,
    u32,
    f32,
    f64
};

using list_type = std::pair<primitive_type, primitive_type>;
using data_type = std::variant<primitive_type, list_type>;

using primitive = std::variant<i8, u8, i16, u16, i32, u32, float, double>;
using list = std::vector<primitive>;
using data = std::variant<primitive, list>;

struct property {
    std::string name;
    data_type data_type;
};

struct element {
    std::string name;
    usize count;
    std::vector<property> properties;
};

struct header {
    std::vector<std::string> comments {};
    std::vector<element> elements {};
};

namespace detail {

struct empty_line {};
struct magic_line {};
using format_line = std::pair<std::string, std::pair<usize, usize>>;
using element_line = std::pair<std::string, usize>;
using property_line = std::pair<std::string, data_type>;
using comment_line = std::string;

using line = std::variant<empty_line, magic_line, format_line, element_line, property_line, comment_line>;

constexpr auto tokenize(std::string_view line) -> std::vector<std::string> {
    return line | ranges::views::split(' ') | ranges::to<std::vector<std::string>>;
};

constexpr auto parse_primitive_type(std::string_view str) -> std::optional<primitive_type> {
    if (str == "char") {
        return primitive_type::s8;
    } else if (str == "uchar") {
        return primitive_type::u8;
    } else if (str == "short") {
        return primitive_type::s16;
    } else if (str == "ushort") {
        return primitive_type::u16;
    } else if (str == "int") {
        return primitive_type::s32;
    } else if (str == "uint") {
        return primitive_type::u32;
    } else if (str == "float") {
        return primitive_type::f32;
    } else if (str == "double") {
        return primitive_type::f64;
    }

    return std::nullopt;
};

constexpr auto parse_primitive(primitive_type expected_type, std::string_view token) -> std::optional<primitive> {
    primitive ret;

    auto parse_and_assign = [&ret, token]<typename T>() -> bool {
        T v;
        if (auto res = std::from_chars(token.data(), token.data() + token.size(), v); res.ec != std::errc()) {
            return false;
        }

        ret = v;
        return true;
    };

    bool res;

    switch (expected_type) {
        case primitive_type::s8: res = parse_and_assign.operator()<i8>(); break;
        case primitive_type::u8: res = parse_and_assign.operator()<u8>(); break;
        case primitive_type::s16: res = parse_and_assign.operator()<i16>(); break;
        case primitive_type::u16: res = parse_and_assign.operator()<u16>(); break;
        case primitive_type::s32: res = parse_and_assign.operator()<i32>(); break;
        case primitive_type::u32: res = parse_and_assign.operator()<u32>(); break;
        case primitive_type::f32: res = parse_and_assign.operator()<float>(); break;
        case primitive_type::f64: res = parse_and_assign.operator()<double>(); break;
        default: std::unreachable();
    }

    if (!res) {
        return std::nullopt;
    }

    return ret;
}

constexpr auto parse_line(std::string_view str) -> stf::expected<line, std::string_view> {
    if (str.empty()) {
        return empty_line{};
    }

    if (str.starts_with("comment")) {
        str.remove_prefix(std::min(str.size(), str.find(' ')));
        return comment_line{str};
    }

    if (str == "ply") {
        return magic_line{};
    }

    std::vector<std::string> tokens = tokenize(str);

    if (tokens.empty()) {
        return empty_line{};
    }

    if (tokens.front() == "format") {
        format_line ret{};

        if (tokens.size() != 3) {
            return stf::unexpected{"expected exactly 3 tokens for a format line"};
        }

        ret.first = tokens[1];

        ret.second = {1, 0};// TODO

        return ret;
    }

    if (tokens.front() == "element") {
        element_line ret{};

        if (tokens.size() != 3) {
            return stf::unexpected{"expected exactly 3 tokens for an element line"};
        }

        ret.first = std::move(tokens[1]);

        if (auto res = std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), ret.second); res.ec != std::errc()) {
            return stf::unexpected{"bad element count"};
        }

        return ret;
    }

    if (tokens.front() == "property") {
        if (tokens.size() < 3) {
            return stf::unexpected{"expected at least 3 tokens for a property line"};
        }

        property_line ret;
        data_type data_type;

        if (tokens[1] == "list") {
            primitive_type index_type;
            primitive_type value_type;

            if (tokens.size() != 5) {
                return stf::unexpected{"expected exactly 5 tokens for a list property line"};
            }

            if (auto res = parse_primitive_type(tokens[2]); !res || (index_type = *res, false)) {
                return stf::unexpected{"expected a primitive type (index type for `list`) as the second token"};
            } else if (res = parse_primitive_type(tokens[3]); !res || (value_type = *res, false)) {
                return stf::unexpected{"expected a primitive type (value type for `list`) as the second token"};
            }

            data_type = std::pair{index_type, value_type};
        } else {
            primitive_type value_type;
            if (auto res = parse_primitive_type(tokens[1]); !res || (value_type = *res, false)) {
                return stf::unexpected{"expected a primitive type as the second token"};
            }

            data_type = value_type;
        }

        ret.first = tokens.back();
        ret.second = data_type;

        return ret;
    }

    return stf::unexpected{"unknown line type"};
}

}// namespace detail

constexpr auto read_header(std::basic_istream<char>& range) -> stf::expected<header, std::string_view> {
    namespace views = ranges::views;

    auto lines = ranges::getlines(range, '\n');
    auto lines_it = lines.begin();
    auto lines_end = lines.end();

    std::vector<detail::line> header_lines;

    for (; lines_it != lines_end; lines_it++) {
        std::string cur_line = *lines_it;
        if (cur_line == "end_header") {
            break;
        }

        header_lines.emplace_back(TRYX(detail::parse_line(cur_line)));
    }

    if (header_lines.size() < 2) {
        return stf::unexpected{"expected at least 2 lines in the header"};
    }

    if (!holds_alternative<detail::magic_line>(header_lines[0])) {
        return stf::unexpected{"first line should be the magic line"};
    }

    if (!holds_alternative<detail::format_line>(header_lines[1])) {
        return stf::unexpected{"second line should be the format line"};
    }

    if (auto line = get<detail::format_line>(header_lines[1]); line.first != "ascii" || line.second.first != 1 || line.second.second != 0) {
        return stf::unexpected{"unsupported PLY format"};
    }

    std::vector<element> elements{};
    std::vector<std::string> comments {};

    for (usize i = 2; i < header_lines.size(); i++) {
        detail::line line = header_lines[i];

        if (std::holds_alternative<detail::comment_line>(line)) {
            comments.emplace_back(std::move(std::get<detail::comment_line>(line)));
            continue;
        }

        if (std::holds_alternative<detail::empty_line>(line)) {
            continue;
        }

        if (!std::holds_alternative<detail::element_line>(line)) {
            return stf::unexpected{"expected element line"};
        }

        element cur_elem{};

        cur_elem.name = std::move(std::get<detail::element_line>(std::move(line)).first);
        cur_elem.count = std::get<detail::element_line>(line).second;

        for (++i; i < header_lines.size(); i++) {// hehe ++i i++, looks funny :>
            detail::line line = header_lines[i];

            if (std::holds_alternative<detail::comment_line>(line)) {
                comments.emplace_back(std::move(std::get<detail::comment_line>(line)));
            }

            if (std::holds_alternative<detail::empty_line>(line)) {
                continue;
            }

            if (std::holds_alternative<detail::element_line>(line)) {
                --i;
                break;
            }

            if (!std::holds_alternative<detail::property_line>(line)) {
                return stf::unexpected{"expected property line"};
            }

            detail::property_line prop_line = std::get<detail::property_line>(line);

            cur_elem.properties.emplace_back(property{
              .name = std::move(prop_line.first),
              .data_type = prop_line.second,
            });
        }

        elements.emplace_back(std::move(cur_elem));
    }

    return header {
      .comments = std::move(comments),
      .elements = std::move(elements),
    };
}

template<typename Fn>
constexpr auto read_element(element description, std::basic_istream<char>& range, Fn&& fn) -> stf::expected<void, std::string_view> {
    auto lines = ranges::getlines(range, '\n');
    auto lines_it = lines.begin();
    auto lines_end = lines.end();

    for (usize i = 0; i < description.count; i++) {
        if (lines_it == lines_end) {
            return stf::unexpected { "unexpected EOF" };
        }

        std::string line = *lines_it++;
        std::vector<std::string> tokens = detail::tokenize(line);
        std::span<std::string> tokens_span { tokens };

        std::vector<data> arguments {};

        for (property prop : description.properties) {
            if (tokens_span.empty()) {
                return stf::unexpected { "ran out of tokens before reading all of the properties" };
            }

            if (std::holds_alternative<primitive_type>(prop.data_type)) {
                if (auto res = detail::parse_primitive(std::get<primitive_type>(prop.data_type), tokens_span.front()); res) {
                    arguments.emplace_back(*res);
                } else {
                    return stf::unexpected { "could not read a property" };
                }
                tokens_span = tokens_span.subspan(1);
            } else {
                auto [size_type, value_type] = std::get<std::pair<primitive_type, primitive_type>>(prop.data_type);

                usize sz;

                // force a primitive type of u32 for sizes?
                if (auto res = detail::parse_primitive(size_type, tokens_span.front()); res) {
                    std::visit([&sz](auto v) { sz = static_cast<usize>(v); }, *res);
                } else {
                    return stf::unexpected { "could not read a property index type" };
                }
                tokens_span = tokens_span.subspan(1);

                if (tokens_span.size() < sz) {
                    return stf::unexpected { "not enough elements in list" };
                }

                list arg {};

                for (usize j = 0; j < sz; j++) {
                    if (auto res = detail::parse_primitive(value_type, tokens_span.front()); res) {
                        arg.emplace_back(*res); // FIXME: 6-deep indentation
                    } else {
                        return stf::unexpected { "could not read a list element" };
                    }
                    tokens_span = tokens_span.subspan(1);
                }

                arguments.emplace_back(std::move(arg));
            }
        }

        std::invoke(std::forward<Fn>(fn), std::move(arguments));
    }

    return {};
}

}// namespace trc::io::ply
