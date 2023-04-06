#include <stuff/ranvec.hpp>
#include <stuff/trilang.hpp>

#include <tracer/bvh/tree.hpp>
#include <tracer/camera/envrionment.hpp>
#include <tracer/camera/pinhole.hpp>
#include <tracer/colormap.hpp>
#include <tracer/common.hpp>
#include <tracer/imgui.hpp>
#include <tracer/integrator/unidirectional_pt.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <imgui-SFML.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <shared_mutex>

using namespace trc;

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

static auto read_stl(std::filesystem::path filename, usize mat_idx, mat4x4 transform = identity_matrix<real, 4, matrix>()) -> std::unique_ptr<dyn_shape> {
    std::ifstream stl_file(filename);
    stf::trilang::stl::binary_stream triangle_stream{std::istreambuf_iterator<char>(stl_file), std::istreambuf_iterator<char>()};

    std::vector<bound_shape> teapot_triangles{};
    for (;;) {
        auto res = triangle_stream.next();
        if (!res) {
            break;
        }

        vec3 v_0 = homogeneous_to_cartesian(transform * vec4{res->vertices[0][0], res->vertices[0][1], res->vertices[0][2], 1});
        vec3 v_1 = homogeneous_to_cartesian(transform * vec4{res->vertices[1][0], res->vertices[1][1], res->vertices[1][2], 1});
        vec3 v_2 = homogeneous_to_cartesian(transform * vec4{res->vertices[2][0], res->vertices[2][1], res->vertices[2][2], 1});

        teapot_triangles.emplace_back(shapes::triangle(mat_idx, {v_0, v_1, v_2}));
    }

    return std::make_unique<bvh_tree>(std::move(teapot_triangles));
}

static auto get_scene_test() -> scene {
    std::vector<material> materials{};

    materials.reserve(32);// avoid reallocations for indices to be reliably gathered

#define PUSH_MATERIAL(...) std::distance(materials.data(), std::addressof(materials.emplace_back(__VA_ARGS__)))

    u32 midx_red_on = PUSH_MATERIAL(materials::oren_nayar(20. / 180. * std::numbers::pi_v<real>, vec3{.75, .25, .25}, vec3(0)));
    u32 midx_blue_on = PUSH_MATERIAL(materials::oren_nayar(20. / 180. * std::numbers::pi_v<real>, vec3{.25, .25, .75}, vec3(0)));
    u32 midx_white_on = PUSH_MATERIAL(materials::oren_nayar(20. / 180. * std::numbers::pi_v<real>, vec3(.75), vec3(0)));
    u32 midx_red = PUSH_MATERIAL(materials::lambertian(vec3{.75, .25, .25}, vec3(0)));
    u32 midx_green = PUSH_MATERIAL(materials::lambertian(vec3{.25, .75, .25}, vec3(0)));
    u32 midx_blue = PUSH_MATERIAL(materials::lambertian(vec3{.25, .25, .75}, vec3(0)));
    u32 midx_white = PUSH_MATERIAL(materials::lambertian(vec3(.75), vec3(0)));

    u32 midx_normal_light = PUSH_MATERIAL(materials::lambertian(vec3(0), uv_albedo{.scale = vec3(15)}));
    u32 midx_white_light = PUSH_MATERIAL(materials::lambertian(vec3(0), vec3(15)));
    u32 midx_red_light = PUSH_MATERIAL(materials::lambertian(vec3(0), vec3{12, 0, 0}));
    u32 midx_green_light = PUSH_MATERIAL(materials::lambertian(vec3(0), vec3{0, 12, 0}));
    u32 midx_blue_light = PUSH_MATERIAL(materials::lambertian(vec3(0), vec3{0, 0, 12}));

    u32 midx_uv = PUSH_MATERIAL(materials::lambertian(uv_albedo{}, vec3(0)));
    u32 midx_uv0 = PUSH_MATERIAL(materials::lambertian(texture("uv_grid_0.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_uv1 = PUSH_MATERIAL(materials::lambertian(texture("uv_grid_1.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_uv2 = PUSH_MATERIAL(materials::lambertian(texture("uv_grid_2.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_surf = PUSH_MATERIAL(materials::lambertian(texture("kodim10.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_parrot = PUSH_MATERIAL(materials::lambertian(texture("kodim23.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_mirror = PUSH_MATERIAL(materials::fresnel_conductor(vec3(0.999), vec3(0)));
    u32 midx_glass = PUSH_MATERIAL(materials::fresnel_dielectric(1, 1.5, vec3(0.999), vec3(0)));

#undef PUSH_MATERIAL

    real obj_radius = 0.85;
    vec3 left_obj_center{-1.3, -2.25 + obj_radius, 8.5};
    vec3 right_obj_center{1.3, -2.25 + obj_radius, 7.3};

    std::vector<bound_shape> bound_shapes{
      //shapes::box(midx_mirror, {left_obj_center - vec3(obj_radius), left_obj_center + vec3(obj_radius)}),
      shapes::sphere(midx_mirror, left_obj_center, obj_radius),
      shapes::sphere(midx_glass, right_obj_center, obj_radius),

      shapes::disc(midx_white_light, {0, 2.2499, 7.5}, {0, -1, 0}, 0.71),
      //shapes::sphere(midx_normal_light, right_obj_center, 0.25),
      /*shapes::disc(midx_red_light, {-0.9, 2.2499, 8}, {0, -1, 0}, 0.71),
      shapes::disc(midx_green_light, {0, 2.2499, 6.5}, {0, -1, 0}, 0.71),
      shapes::disc(midx_blue_light, {0.9, 2.2499, 8}, {0, -1, 0}, 0.71),*/

      //shapes::box(midx_magdonal, {{-2.5, -2.25, 5}, {-0.5, -1.25, 7}}),

      //shapes::triangle(midx_green, {{left_obj_center + vec3{0, 0 - obj_radius, -2}, left_obj_center + vec3{1, 0 - obj_radius, -2}, left_obj_center + vec3{0, 1 - obj_radius, -2}}}),
    };

    std::vector<std::unique_ptr<dyn_shape>> dyn_shapes{};
    dyn_shapes.emplace_back(std::make_unique<bvh_tree>(std::move(bound_shapes)));

    /*homogenous_transformation<real> teapot_transformation_0{
      .rotation{std::numbers::pi_v<real> * -90 / 180, 0, std::numbers::pi_v<real> * 135 / 180},
      .scale{0.1666, 0.1666, 0.1666},
      .translation{right_obj_center[0], right_obj_center[1] - obj_radius * 0.75f, right_obj_center[2]},
    };

    mat4x4 teapot_mat_0 = homogeneous_transformation_matrix<real, matrix>(teapot_transformation_0);
    dyn_shapes.emplace_back(read_stl("Utah_teapot_(solid).stl", midx_glass, teapot_mat_0));

    homogenous_transformation<real> teapot_transformation_1{
      .rotation{std::numbers::pi_v<real> * -90 / 180, 0, std::numbers::pi_v<real> * 45 / 180},
      .scale{0.1666, 0.1666, 0.1666},
      .translation{left_obj_center[0], left_obj_center[1] - obj_radius * 0.75f, left_obj_center[2]},
    };

    mat4x4 teapot_mat_1 = homogeneous_transformation_matrix<real, matrix>(teapot_transformation_1);
    dyn_shapes.emplace_back(read_stl("Utah_teapot_(solid).stl", midx_mirror, teapot_mat_1));*/

    std::vector<shape> shapes{
      shapes::plane(midx_red, {-2.8, 0, 10}, {1, 0, 0}),   // left
      shapes::plane(midx_white, {0, 0, 10}, {0, 0, -1}),   // back
      shapes::plane(midx_blue, {2.8, 0, 10}, {-1, 0, 0}),  // right
      shapes::plane(midx_white, {0, 2.25, 10}, {0, -1, 0}),// top
      shapes::plane(midx_white, {0, -2.25, 10}, {0, 1, 0}),// bottom
    };

    scene scene(std::move(shapes), std::move(materials), std::move(dyn_shapes));

    return scene;
}

struct camera_settings {
    real fov;
    vec3 center;
    vec3 rotation;

    auto operator()(sf::Vector2u window_dimensions) const -> std::shared_ptr<camera> {
        vec2 dimensions(window_dimensions.x, window_dimensions.y);

        vec3 rad_rotation = rotation / 180 * std::numbers::pi_v<real>;
        mat3x3 mat = rotation_matrix<real, matrix>(rotation[0], rotation[1], rotation[2]);

        return std::make_unique<pinhole_camera>(center, dimensions, fov / 180 * std::numbers::pi_v<real>, mat);
    }
};

struct render_request {
    integration_settings settings;
    std::shared_ptr<scene> scene;
    camera_settings cam_settings;
    image_view out;
};

struct renderer {
    renderer()
        : m_window(sf::VideoMode(m_resolution), "tracer", sf::Style::Titlebar | sf::Style::Close)
        , m_image(m_resolution.x, m_resolution.y)
        , m_render_thread([this] { render_worker(); })
        , m_image_fetch_thread([this] { image_fetch_worker(); }) {
    }

    int main() {
        m_window.setFramerateLimit(60);
        ImGui::SFML::Init(m_window);

        for (sf::Clock frame_delta_clock{}, image_fetch_clock{}; m_window.isOpen();) {
            for (sf::Event event; m_window.pollEvent(event);) {
                ImGui::SFML::ProcessEvent(m_window, event);

                if (event.type == sf::Event::Closed)
                    m_window.close();
            }

            ImGui::SFML::Update(m_window, frame_delta_clock.restart());

            ImGui::Begin("controls");

            ImGui::PushItemWidth(64);

            bool resolution_changed = false;
            resolution_changed |= ImGui::InputScalar("width", imgui_type_v<unsigned int>, &m_resolution.x);
            ImGui::SameLine();
            resolution_changed |= ImGui::InputScalar("height", imgui_type_v<unsigned int>, &m_resolution.y);

            if (resolution_changed) {
                m_window.setSize(m_resolution);
                m_image.create(m_resolution.x, m_resolution.y);
            }

            ImGui::Separator();

            ImGui::InputScalar("samples", imgui_type_v<usize>, &m_integration_settings.samples);

            ImGui::Separator();

            ImGui::InputScalar("fov (degrees)", imgui_type_v<double>, &m_camera_settings.fov);

            ImGui::PopItemWidth();

            ImGui::PushItemWidth(64 * 3);

            ImGui::InputScalarN("camera position", imgui_type_v<double>, &m_camera_settings.center, 3);
            ImGui::InputScalarN("camera rotation", imgui_type_v<double>, &m_camera_settings.rotation, 3);

            ImGui::PopItemWidth();

            ImGui::Separator();

            if (ImGui::Button("begin render")) {
                request_render();
            }

            ImGui::End();

            if (image_fetch_clock.getElapsedTime().asMilliseconds() >= 250) {
                image_fetch_clock.restart();
                request_image_fetch();
            }

            m_window.clear();

            std::unique_lock sprite_lock{m_texture_sprite_mutex};
            m_window.draw(m_sf_sprite);

            ImGui::SFML::Render(m_window);
            m_window.display();
        }

        m_render_complete_channel.close();
        m_render_request_channel.close();

        m_image_request_channel.close();
        m_image_complete_channel.close();

        m_render_thread.join();
        m_image_fetch_thread.join();

        ImGui::SFML::Shutdown();

        return 0;
    }

private:
#ifdef NDEBUG
    sf::Vector2u m_resolution{1024, 768};
#else
    sf::Vector2u m_resolution{400, 300};
#endif

    integration_settings m_integration_settings{
#ifdef NDEBUG
      .samples = 4,
#else
      .samples = 1,
#endif
    };

    camera_settings m_camera_settings{
      .fov = 80,
      .center{0, 0, 0},
      .rotation{0, 0, 0},
    };

    std::shared_ptr<scene> m_scene_to_render = std::make_shared<scene>(std::move(get_scene_test()));

    sf::RenderWindow m_window;

    image m_image{};
    sf::Image m_sf_image{};
    std::mutex m_texture_sprite_mutex{};
    sf::Texture m_sf_texture{};
    sf::Sprite m_sf_sprite{};

    stf::channel<render_request, 0> m_render_request_channel{};
    stf::channel<void> m_render_complete_channel{};
    std::thread m_render_thread;

    stf::channel<void, 0> m_image_request_channel{};
    stf::channel<void, 0> m_image_complete_channel{};
    std::thread m_image_fetch_thread;

    void request_render() {
        if (stf::select(
              stf::channel_selector(m_render_complete_channel, [](auto) { return false; }),
              stf::default_channel_selector{[] { return true; }}//
              )) {
            spdlog::warn("cannot request another render as there's an ongoing one");
            return;
        }

        if (m_image.width() != m_resolution.x || m_image.height() != m_resolution.y) {
            m_image.create(m_resolution.x, m_resolution.y);
        }

        m_render_request_channel.emplace_back(render_request{
          .settings = m_integration_settings,
          .scene = m_scene_to_render,
          .cam_settings = m_camera_settings,
          .out = m_image,
        });
    }

    void render_worker() {
        stf::random::xoshiro_256p gen{std::random_device{}()};

        for (;;) {
            m_render_complete_channel.emplace_back();

            auto res = m_render_request_channel.pop_front();
            if (!res) {
                spdlog::info("render channel closed");
                break;
            }

            render_request request = *res;

            spdlog::info("render request received");

            std::shared_ptr<camera> camera = request.cam_settings(sf::Vector2u(request.out.width(), request.out.height()));
            unidirectional_pt integrator(std::move(camera), request.scene);

            integrator.integrate(request.out, request.settings, gen);

            stf::qoi::image qoi_image{};
            qoi_image.create(request.out.width(), request.out.height());

            for (usize y = 0; y < request.out.height(); y++) {
                for (usize x = 0; x < request.out.width(); x++) {
                    color c = srgb_oetf(request.out.at(x, y));

                    qoi_image.at(x, y) = stf::qoi::color{
                      static_cast<u8>(std::round(std::clamp(c[0], real(0), real(1)) * 255)),
                      static_cast<u8>(std::round(std::clamp(c[1], real(0), real(1)) * 255)),
                      static_cast<u8>(std::round(std::clamp(c[2], real(0), real(1)) * 255)),
                      255,
                    };
                }
            }

            qoi_image.to_file("render.qoi");

            spdlog::info("render completed in {} seconds", 0);
        }

        spdlog::info("render thread is exiting");
    }

    void request_image_fetch() {
        if (stf::select(
              stf::channel_selector(m_image_complete_channel, [](auto) { return false; }),
              stf::default_channel_selector{[] { return true; }}//
              )) {
            spdlog::warn("cannot request another image fetch as there's an ongoing one (basically an overrun)");
            return;
        }

        m_image_request_channel.emplace_back();
    }

    void image_fetch_worker() {
        for (;;) {
            m_image_complete_channel.emplace_back();

            auto res = m_image_request_channel.pop_front();
            if (!res) {
                spdlog::info("image fetch channel closed");
                break;
            }

            if (m_sf_image.getSize().x != m_image.width() || m_sf_image.getSize().y != m_image.height()) {
                m_sf_image.create(sf::Vector2u(m_image.width(), m_image.height()));
            }

            for (u32 row = 0; row < m_image.height(); row++) {
                for (u32 col = 0; col < m_image.width(); col++) {
                    color c = round(clamp(srgb_oetf(m_image.at(col, row)), 0, 1) * 255);

                    sf::Color color{
                      std::isnan(c[0]) ? u8(255) : static_cast<u8>(c[0]),
                      std::isnan(c[1]) ? u8(255) : static_cast<u8>(c[1]),
                      std::isnan(c[2]) ? u8(255) : static_cast<u8>(c[2]),
                      255,
                    };

                    m_sf_image.setPixel({col, row}, color);
                }
            }

            std::unique_lock texture_sprite_lock{m_texture_sprite_mutex};
            m_sf_texture.loadFromImage(m_sf_image);
            m_sf_sprite.setTexture(m_sf_texture);
        }

        spdlog::info("image fetch thread is exiting");
    }
};

int main() {
    renderer renderer{};
    return renderer.main();
}
