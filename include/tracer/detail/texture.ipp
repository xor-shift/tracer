#pragma once

namespace trc {

template<typename Allocator>
constexpr void texture::create(stf::qoi::image<Allocator>&& image, wrapping_mode wrapping_mode, scaling_method scaling_method) {
    m_image = std::move(image);
    m_wrapping_mode = wrapping_mode;
    m_scaling_method = scaling_method;

    m_width = static_cast<real>(m_image.width());
    m_height = static_cast<real>(m_image.width());
}

auto texture::from_file(std::string_view filename, wrapping_mode wrapping_mode, scaling_method scaling_method) -> stf::expected<void, std::string_view> {
    stf::qoi::image<> image_copy = m_image;

    stf::scope::scope_exit guard{[&] {
        m_image = std::move(image_copy);
    }};

    TRYX(m_image.from_file(filename));

    guard.release();

    m_wrapping_mode = wrapping_mode;
    m_scaling_method = scaling_method;

    m_width = static_cast<real>(m_image.width());
    m_height = static_cast<real>(m_image.width());

    return {};
}

constexpr auto texture::sample(vec2 uv) const -> color {
    if (empty()) {
        return color{};
    }

    mat2x2 dims{static_cast<real>(m_image.width() - 1), 0, 0, static_cast<real>(m_image.height() - 1)};
    vec2 scaled = dims * uv;

    switch (m_scaling_method) {
        case scaling_method::nearest:
            return sample_nearest(scaled);
        case scaling_method::bilinear: {
            vec3 uv_00 = {std::floor(scaled[0]), std::floor(scaled[1])};
            vec3 uv_11 = {std::ceil(scaled[0]), std::ceil(scaled[1])};

            real u_param = scaled[0] - uv_00[0];
            real v_param = scaled[1] - uv_00[1];

            color q_00 = sample_nearest({uv_00[0], uv_00[1]});
            color q_01 = sample_nearest({uv_11[0], uv_00[1]});
            color q_10 = sample_nearest({uv_00[0], uv_11[1]});
            color q_11 = sample_nearest({uv_11[0], uv_11[1]});

            color u_interp = q_00 * (1 - u_param) + q_01 * u_param;
            color v_interp = q_10 * (1 - u_param) + q_11 * u_param;

            return u_interp * (1 - v_param) + v_interp * v_param;
        };
        case scaling_method::bicubic: {
            vec3 uv_00 = {std::floor(scaled[0]), std::floor(scaled[1])};
            vec3 uv_11 = {std::ceil(scaled[0]), std::ceil(scaled[1])};

            real u_param = scaled[0] - uv_00[0];
            real v_param = scaled[1] - uv_00[1];

            color q_00 = sample_nearest({uv_00[0] - 1, uv_00[1] - 1});
            color q_01 = sample_nearest({uv_00[0], uv_00[1] - 1});
            color q_02 = sample_nearest({uv_11[0], uv_00[1] - 1});
            color q_03 = sample_nearest({uv_11[0] + 1, uv_00[1] - 1});

            color q_10 = sample_nearest({uv_00[0] - 1, uv_00[1]});
            color q_11 = sample_nearest({uv_00[0], uv_00[1]});
            color q_12 = sample_nearest({uv_11[0], uv_00[1]});
            color q_13 = sample_nearest({uv_11[0] + 1, uv_00[1]});

            color q_20 = sample_nearest({uv_00[0] - 1, uv_11[1]});
            color q_21 = sample_nearest({uv_00[0], uv_11[1]});
            color q_22 = sample_nearest({uv_11[0], uv_11[1]});
            color q_23 = sample_nearest({uv_11[0] + 1, uv_11[1]});

            color q_30 = sample_nearest({uv_00[0] - 1, uv_11[1] + 1});
            color q_31 = sample_nearest({uv_00[0], uv_11[1] + 1});
            color q_32 = sample_nearest({uv_11[0], uv_11[1] + 1});
            color q_33 = sample_nearest({uv_11[0] + 1, uv_11[1] + 1});

            auto cerp = [](color x0, color x1, color x2, color x3, real t) -> color {
                color a = (x3 - x2) - (x0 - x1);
                color b = (x0 - x1) - a;
                color c = x2 - x0;
                color d = x1;

                real t2 = t * t;
                real t3 = t2 * t;

                return a * t3 + b * t2 + c * t + d;
            };

            color ui_0 = cerp(q_00, q_01, q_02, q_03, u_param);
            color ui_1 = cerp(q_10, q_11, q_12, q_13, u_param);
            color ui_2 = cerp(q_20, q_21, q_22, q_23, u_param);
            color ui_3 = cerp(q_30, q_31, q_32, q_33, u_param);

            return cerp(ui_0, ui_1, ui_2, ui_3, v_param);
        };
    }

    return sample_nearest(dims * uv);
}

constexpr auto texture::sample_nearest(vec2 xy) const -> color {
    switch (m_wrapping_mode) {
        case wrapping_mode::extend:
            xy[0] = std::clamp(xy[0], real(0), m_width - 1);
            xy[1] = std::clamp(xy[1], real(0), m_height - 1);
            break;

        case wrapping_mode::repeat:
            xy[0] = std::fabs(std::fmod(xy[0], m_width - 1));
            xy[1] = std::fabs(std::fmod(xy[1], m_height - 1));
            break;

        default:
            std::unreachable();
    }

    stf::qoi::color qoi_albedo = m_image.at(static_cast<u32>(xy[0]), static_cast<u32>(xy[1]));

    return color{
      static_cast<real>(qoi_albedo.r) / 255,
      static_cast<real>(qoi_albedo.g) / 255,
      static_cast<real>(qoi_albedo.b) / 255,
      static_cast<real>(qoi_albedo.a) / 255,
    };
}

}
