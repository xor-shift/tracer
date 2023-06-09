#include <tracer/run/sfml/main.hpp>

#include <tracer/bvh/tree.hpp>
#include <tracer/camera/envrionment.hpp>
#include <tracer/camera/orthographic.hpp>
#include <tracer/camera/perspective.hpp>
#include <tracer/camera/pinhole.hpp>
#include <tracer/image.hpp>
#include <tracer/imgui.hpp>
#include <tracer/integrator/cosine_albedo.hpp>
#include <tracer/integrator/unidirectional_pt.hpp>
#include <tracer/io/ply.hpp>
#include <tracer/io/stl.hpp>
#include <tracer/scene.hpp>

#include <SFML/Graphics.hpp>

#include <imgui-SFML.h>
#include <spdlog/spdlog.h>

namespace trc {

template<std::unsigned_integral IndexType = u32>
static auto read_stl(std::filesystem::path filename, u32 mat_idx, mat4x4 transform = mat4x4::identity()) -> shapes::mesh<IndexType> {
    std::ifstream stl_file(filename);
    trc::io::stl::binary_stream triangle_stream{std::istreambuf_iterator<char>(stl_file), std::istreambuf_iterator<char>()};

    shapes::mesh<IndexType> mesh{mat_idx};

    for (;;) {
        auto res = triangle_stream.next();
        if (!res) {
            break;
        }

        mesh.push_triangle({res->vertices[0], res->vertices[1], res->vertices[2]});
    }

    mesh.transform(transform);
    mesh.finish_construction();

    return mesh;
}

template<std::unsigned_integral IndexType = u32>
static auto read_ply(std::filesystem::path filename, u32 mat_idx, mat4x4 transform = mat4x4::identity()) -> shapes::mesh<IndexType> {
    std::ifstream ifs("bun_zipper.ply");
    auto res_0 = io::ply::read_header(ifs);

    shapes::mesh<IndexType> mesh{mat_idx};

    auto get_real = []<typename T>(T const& v) -> real {
        io::ply::primitive p_v;
        if constexpr (std::is_same_v<T, io::ply::data>) {
            p_v = std::get<io::ply::primitive>(v);
        } else if constexpr (std::is_same_v<T, io::ply::primitive>) {
            p_v = v;
        } else {
            std::unreachable();
        }
        return std::visit([](auto v) -> real { return static_cast<real>(v); }, p_v);
    };

    auto res_1 = io::ply::read_element(res_0->elements[0], ifs, [&](std::vector<io::ply::data> args) {
        mesh.push_vertex(vec3(get_real(args[0]), get_real(args[1]), get_real(args[2])));
    });

    auto res_2 = io::ply::read_element(res_0->elements[1], ifs, [&](std::vector<io::ply::data> args) {
        io::ply::list list = std::get<io::ply::list>(args[0]);
        mesh.push_triangle(std::array<IndexType, 3>{
          static_cast<IndexType>(std::get<int>(list[0])),
          static_cast<IndexType>(std::get<int>(list[1])),
          static_cast<IndexType>(std::get<int>(list[2])),
        });
    });

    mesh.transform(transform);
    mesh.finish_construction();

    return mesh;
}

static auto get_scene_test() -> scene {
    scene scene{};

    u32 midx_red_on = scene.add_material(materials::oren_nayar(20. / 180. * std::numbers::pi_v<real>, vec3{.75, .25, .25}, vec3(0)));
    u32 midx_blue_on = scene.add_material(materials::oren_nayar(20. / 180. * std::numbers::pi_v<real>, vec3{.25, .25, .75}, vec3(0)));
    u32 midx_white_on = scene.add_material(materials::oren_nayar(20. / 180. * std::numbers::pi_v<real>, vec3(.75), vec3(0)));
    u32 midx_red = scene.add_material(materials::lambertian(vec3{.75, .25, .25}, vec3(0)));
    u32 midx_green = scene.add_material(materials::lambertian(vec3{.25, .75, .25}, vec3(0)));
    u32 midx_blue = scene.add_material(materials::lambertian(vec3{.25, .25, .75}, vec3(0)));
    u32 midx_white = scene.add_material(materials::lambertian(vec3(.75), vec3(0)));

    u32 midx_normal_light = scene.add_material(materials::lambertian(vec3(0), uv_albedo{.scale = vec3(15)}));
    u32 midx_white_light = scene.add_material(materials::lambertian(vec3(0), vec3(15)));
    u32 midx_red_light = scene.add_material(materials::lambertian(vec3(0), vec3{12, 0, 0}));
    u32 midx_green_light = scene.add_material(materials::lambertian(vec3(0), vec3{0, 12, 0}));
    u32 midx_blue_light = scene.add_material(materials::lambertian(vec3(0), vec3{0, 0, 12}));

    u32 midx_uv = scene.add_material(materials::lambertian(uv_albedo{}, vec3(0)));
    u32 midx_uv0 = scene.add_material(materials::lambertian(texture("uv_grid_0.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_uv1 = scene.add_material(materials::lambertian(texture("uv_grid_1.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_uv2 = scene.add_material(materials::lambertian(texture("uv_grid_2.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_surf = scene.add_material(materials::lambertian(texture("kodim10.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_parrot = scene.add_material(materials::lambertian(texture("kodim23.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_mirror = scene.add_material(materials::fresnel_conductor(vec3(0.999), vec3(0)));
    u32 midx_glass = scene.add_material(materials::fresnel_dielectric(1, 1.5, vec3(0.999), vec3(0)));

    real obj_radius = 0.85;
    vec3 left_obj_center{-1.3, -2.25 + obj_radius, 8.5};
    vec3 right_obj_center{1.3, -2.25 + obj_radius, 7.3};

    // scene.append_shape(shapes::sphere(midx_mirror, left_obj_center, obj_radius));
    // scene.append_shape(shapes::sphere(midx_glass, right_obj_center, obj_radius));
    // scene.append_shape(shapes::disc(midx_white_light, {0, 2.2499, 7.5}, {0, -1, 0}, 0.71));
    scene.append_shape(shapes::disc(midx_white_light, {-1, 0.2499, 7.5}, normalize(vec3{1, -1, 0}), 0.71));
    scene.append_shape(shapes::disc(midx_white, {-1, 0.25001, 7.5}, normalize(vec3{1, -1, 0}), 0.72));

    // rgb lights
    /*scene.append_shape(shapes::disc(midx_red_light, {-0.9, 2.2499, 8}, {0, -1, 0}, 0.71));
    scene.append_shape(shapes::disc(midx_green_light, {0, 2.2499, 6.5}, {0, -1, 0}, 0.71));
    scene.append_shape(shapes::disc(midx_blue_light, {0.9, 2.2499, 8}, {0, -1, 0}, 0.71));*/

    // scene.append_shape(shapes::box(midx_mirror, {left_obj_center - vec3(obj_radius), left_obj_center + vec3(obj_radius)}));
    // scene.append_shape(shapes::sphere(midx_normal_light, right_obj_center, 0.25));
    // scene.append_shape(shapes::box(midx_magdonal, {{-2.5, -2.25, 5}, {-0.5, -1.25, 7}}));
    // scene.append_shape(shapes::triangle(midx_green, {{left_obj_center + vec3{0, 0 - obj_radius, -2}, left_obj_center + vec3{1, 0 - obj_radius, -2}, left_obj_center + vec3{0, 1 - obj_radius, -2}}}));

    mat4x4 teapot_mat_0 =
      mat4x4::translate(right_obj_center[0], right_obj_center[1] - obj_radius * 0.75f, right_obj_center[2]) *
      mat4x4::scale(0.1666, 0.1666, 0.1666) *
      mat4x4::rotate(std::numbers::pi_v<real> * -90 / 180, 0, std::numbers::pi_v<real> * 135 / 180);

    //scene.append_shape(read_stl<u16>("Utah_teapot_(solid).stl", midx_glass, teapot_mat_0));

    mat4x4 teapot_mat_1 =
      mat4x4::translate(left_obj_center[0], left_obj_center[1] - obj_radius * 0.75f, left_obj_center[2]) *
      mat4x4::scale(0.1666, 0.1666, 0.1666) *
      mat4x4::rotate(std::numbers::pi_v<real> * -90 / 180, 0, std::numbers::pi_v<real> * 45 / 180);

    //scene.append_shape(read_stl<u16>("Utah_teapot_(solid).stl", midx_mirror, teapot_mat_1));

    scene.append_shape(read_ply<u32>("", midx_glass, mat4x4::translate(0, -2.75, 8) * mat4x4::scale(15, 15, 15) * mat4x4::rotate(0, std::numbers::pi_v<real>, 0)));

    std::vector<unbound_shape> unbound_shapes{
      shapes::plane(midx_red, {-2.8, 0, 10}, {1, 0, 0}),   // left
      shapes::plane(midx_uv0, {0, 0, 10}, {0, 0, -1}),     // back
      shapes::plane(midx_blue, {2.8, 0, 10}, {-1, 0, 0}),  // right
      shapes::plane(midx_white, {0, 2.25, 10}, {0, -1, 0}),// top
      shapes::plane(midx_white, {0, -2.25, 10}, {0, 1, 0}),// bottom
    };

    scene.append_shapes(std::move(unbound_shapes));
    scene.reconstruct_bvh<binary_bvh<bound_shape>>(12);

    return scene;
}

auto sfml_program::camera_settings::operator()(sf::Vector2u window_dimensions) const -> std::shared_ptr<camera> {
    vec2 dimensions(window_dimensions.x, window_dimensions.y);

    if (type == camera_type::pinhole) {
        vec3 rad_rotation = rotation / 180 * std::numbers::pi_v<real>;
        mat3x3 mat = mat3x3::rotate(rotation[0], rotation[1], rotation[2]);

        return std::make_shared<pinhole_camera>(center, dimensions, fov / 180 * std::numbers::pi_v<real>, mat);
    } else if (type == camera_type::environment) {
        return std::make_shared<environment_camera>(center, dimensions);
    } else if (type == camera_type::orthographic_raw) {
        return std::make_shared<orthographic_camera>(dimensions, center, left, right, bottom, top, near, far, rotation, ray_rotation);
    } else if (type == camera_type::frustum_raw) {
        return std::make_shared<frustum_camera>(dimensions, left, right, bottom, top, near, far);
    } else if (type == camera_type::orthographic) {
        vec2 lr = vec2(-0.5, 0.5) * physical_width;
        vec2 bt = vec2(-0.5, 0.5) * physical_width * dimensions[1] / dimensions[0];

        return std::make_shared<orthographic_camera>(dimensions, center, lr[0], lr[1], bt[0], bt[1], near, far, rotation, ray_rotation);
    } else if (type == camera_type::frustum) {
        return std::make_shared<frustum_camera>(dimensions, left, right, bottom, top, near, far);
    } else {
        std::unreachable();
    }
}

sfml_program::sfml_program()
    : m_scene(std::make_shared<scene>(std::move(get_scene_test())))
    , m_window(sf::VideoMode({1280, 720}), "tracer")
    , m_image(m_configuration.m_resolution.x, m_configuration.m_resolution.y)
    , m_render_thread([this] { render_worker(); }) {

    m_sf_image.create(m_configuration.m_resolution);

    m_imgui_context = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_imgui_context);

    m_window.setFramerateLimit(60);
    ImGui::SFML::Init(m_window);

    //load_font("./DroidSans.ttf", "Droid Sans (TTF)");
    load_fonts();
}

sfml_program::~sfml_program() {
    m_render_complete_channel.close();
    m_render_request_channel.close();

    m_render_thread.join();

    ImGui::GetIO().Fonts->ClearFonts();

    ImGui::SFML::Shutdown();

    m_window.close();
    ImGui::DestroyContext(m_imgui_context);
}

auto sfml_program::run() -> int {
    for (sf::Clock frame_delta_clock{}, image_fetch_clock{}; m_window.isOpen();) {
        for (sf::Event event; m_window.pollEvent(event);) {
            ImGui::SFML::ProcessEvent(m_window, event);

            if (event.type == sf::Event::Closed)
                m_window.close();
        }

        ImGui::SFML::Update(m_window, frame_delta_clock.restart());
        ImGui::PushFont(m_font);

        ImGui::SetNextWindowSize(ImVec2(m_window.getSize().x, m_window.getSize().y));
        ImGui::SetNextWindowPos({0, 0});
        if (ImGui::Begin("tracer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar)) {
            gui_menu_bar();

            ImGui::BeginTable("tracer_main_table", 3, ImGuiTableFlags_Resizable, ImGui::GetContentRegionAvail());
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            gui_render_settings();

            ImGui::TableNextColumn();
            {
                ImGui::BeginChild("image_view", ImVec2(0, ImGui::GetContentRegionAvail().y - 2), false, ImGuiWindowFlags_HorizontalScrollbar);

                vec2 image_size(m_sf_image.getSize().x, m_sf_image.getSize().y);
                vec2 available_region(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
                vec2 cursor_pos = clamp((available_region - image_size) * real(0.5), real(0), infinity);
                ImGui::SetCursorPos(ImVec2(cursor_pos[0], cursor_pos[1]));

                m_sf_texture.loadFromImage(m_sf_image);
                ImGui::Image(m_sf_texture);
                ImGui::EndChild();
            }


            ImGui::TableNextColumn();

            {
                auto group_guard = imgui::guarded_group();

                if (ImGui::BeginTabBar("MaterialObjectEditor")) {
                    if (ImGui::BeginTabItem("Materials")) {
                        if (ImGui::TreeNode("Material Editor")) {
                            gui_render_material_editor();
                            ImGui::TreePop();
                        }

                        if (ImGui::TreeNode("Materials List")) {
                            gui_render_material_list();
                            ImGui::TreePop();
                        }
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Objects")) {
                        if (ImGui::CollapsingHeader("Object Editor")) {
                            gui_render_object_editor();
                        }

                        if (ImGui::CollapsingHeader("Scene Editor")) {
                            gui_render_scene_editor();
                        }
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
            }

            ImGui::EndTable();
        }
        ImGui::End();

        if (m_ui_render_on_invalidate && m_invalidated) {
            m_invalidated = !request_render();
            //request_render();
            //m_invalidated = false;
        }

        m_window.clear();

        ImGui::PopFont();
        ImGui::SFML::Render(m_window);
        m_window.display();
    }

    return 0;
}

void sfml_program::load_fonts() {
    ImGuiIO& io = ImGui::GetIO();
    m_fonts["ImGUI Default"] = io.Fonts->AddFontDefault();
    m_fonts["Proggy Clean (TTF)"] = io.Fonts->AddFontFromFileTTF("ProggyClean.ttf", 13);
    // m_fonts["Proggy Tiny (TTF)"] = io.Fonts->AddFontFromFileTTF("ProggyTiny.ttf", 13);
    m_fonts["Droid Sans (TTF)"] = io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 13);
    m_fonts["Karla Regular (TTF)"] = io.Fonts->AddFontFromFileTTF("Karla-Regular.ttf", 13);
    m_fonts["Cousine Regular (TTF)"] = io.Fonts->AddFontFromFileTTF("Cousine-Regular.ttf", 13);
    io.Fonts->Build();
    ImGui::SFML::UpdateFontTexture();

    m_font = m_fonts["Droid Sans (TTF)"];
}

auto sfml_program::recreate_images() -> bool {
    bool inconsistent = false;

    inconsistent |= m_sf_image.getSize() != m_configuration.m_resolution;
    inconsistent |= m_image.width() != m_configuration.m_resolution.x;
    inconsistent |= m_image.height() != m_configuration.m_resolution.y;
    inconsistent |= m_qoi_image.width() != m_configuration.m_resolution.x;
    inconsistent |= m_qoi_image.height() != m_configuration.m_resolution.y;

    if (!inconsistent) {
        return false;
    }

    m_image.create(m_configuration.m_resolution.x, m_configuration.m_resolution.y);
    m_qoi_image.create(m_configuration.m_resolution.x, m_configuration.m_resolution.y);
    m_sf_image.create(m_configuration.m_resolution);

    return true;
}

auto sfml_program::request_render() -> bool {
    if (stf::select(
          stf::channel_selector(m_render_complete_channel, [](auto) { return false; }),
          stf::default_channel_selector{[] { return true; }}//
          )) {
        spdlog::warn("cannot request another render as there's an ongoing one");
        return false;
    }

    recreate_images();
    send(m_render_request_channel, m_configuration);

    return true;
}

void sfml_program::render_worker() {
    stf::random::xoshiro_256p gen{std::random_device{}()};

    for (;;) {
        m_ongoing_render.store(false, std::memory_order::relaxed);
        send(m_render_complete_channel);

        auto res = receive(m_render_request_channel);
        if (!res) {
            spdlog::info("render channel closed");
            break;
        }

        render_configuration request = *res;

        spdlog::info("render request received");
        m_ongoing_render.store(true, std::memory_order::relaxed);

        std::shared_ptr<camera> camera = request.m_camera_settings(sf::Vector2u(m_image.width(), m_image.height()));
        std::shared_ptr<integrator> integrator = nullptr;

        switch (m_configuration.m_integrator) {
            case integrator_type::cosine_albedo:
                integrator = std::make_shared<cosine_albedo_integrator>(std::move(camera), m_scene);
                break;
            /*case integrator_type::light_visibility:
                    integrator = std::make_shared<light_visibility_checker>(std::move(camera), m_scene);
                    break;*/
            case integrator_type::unidirectional_pt:
                integrator = std::make_shared<unidirectional_pt>(std::move(camera), m_scene);
                break;
            default:
                std::unreachable();
        }

        image_adapter_qoi adapted_qoi_image{m_qoi_image};
        image_adapter_sfml adapted_sf_image{m_sf_image};
        image_adapter_srgb srgb_sf_image{adapted_sf_image};
        image_adapter_tee images_tee{adapted_qoi_image, srgb_sf_image, m_image};

        std::chrono::time_point tp_0 = std::chrono::system_clock::now();
        integrator->integrate(images_tee, request.m_integrator_settings, gen);
        std::chrono::time_point tp_1 = std::chrono::system_clock::now();

        spdlog::info("render completed in {} seconds", std::chrono::duration_cast<std::chrono::microseconds>(tp_1 - tp_0).count() / 1000000.);
    }

    spdlog::info("render thread is exiting");
}

void sfml_program::save_to_file(std::string_view filename) {
    std::chrono::time_point tp_0 = std::chrono::system_clock::now();
    m_qoi_image.to_file(filename);
    std::chrono::time_point tp_1 = std::chrono::system_clock::now();

    spdlog::info("wrote to \"{}\" in {} seconds", filename, std::chrono::duration_cast<std::chrono::microseconds>(tp_1 - tp_0).count() / 1000000.);
}

void sfml_program::gui_menu_bar() {
    if (!ImGui::BeginMenuBar()) {
        return;
    }

    bool popup_about = false;
    bool popup_save_as = false;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    if (ImGui::MenuItem("About")) {
        popup_about = true;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save To render.qoi")) {
            save_to_file("render.qoi");
        }

        if (ImGui::MenuItem("Save As...")) {
            popup_save_as = true;
        }

        if (ImGui::BeginMenu("UI Font...")) {
            for (auto const& [name, font]: m_fonts) {
                if (ImGui::MenuItem(name.c_str())) {
                    m_font = font;
                }
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();

    if (popup_about) {
        ImGui::OpenPopup("About");
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("");
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }

    if (popup_save_as) {
        ImGui::OpenPopup("Save As");
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    if (ImGui::BeginPopupModal("Save As", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("File name", m_filename_buffer.data(), m_filename_buffer.size(), ImGuiInputTextFlags_None);

        if (ImGui::Button("Save", ImVec2(120, 0))) {
            save_to_file({m_filename_buffer.data(), std::strlen(m_filename_buffer.data())});
            ImGui::CloseCurrentPopup();
        }

        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
}

auto sfml_program::gui_render_camera_editor() -> bool {
    bool invalidated = false;

    auto rotation = [&](auto& rotation, std::string_view id_base) {
        ImGui::PushID(fmt::format("{}_1", id_base).c_str());
        ImGui::PushID(fmt::format("{}_2", id_base).c_str());
        ImGui::PushID(fmt::format("{}_0", id_base).c_str());
        invalidated |= imgui::slider_angle<real>("Pitch", rotation[0], -180, 180);
        ImGui::PopID();
        invalidated |= imgui::slider_angle<real>("Yaw", rotation[1], -180, 180);
        ImGui::PopID();
        invalidated |= imgui::slider_angle<real>("Roll", rotation[2]);
        ImGui::PopID();
    };

    if (camera_type type = m_configuration.m_camera_settings.type; type == camera_type::pinhole) {
        invalidated |= imgui::slider<real>("FOV", m_configuration.m_camera_settings.fov, 0, 180);
        rotation(m_configuration.m_camera_settings.rotation, "camera_rot_sliders");
    } else if (type == camera_type::environment) {
        //
    } else if (type == camera_type::orthographic_raw || type == camera_type::frustum_raw) {
        invalidated |= imgui::input_scalar<real>("Left", m_configuration.m_camera_settings.left);
        invalidated |= imgui::input_scalar<real>("Right", m_configuration.m_camera_settings.right);
        invalidated |= imgui::input_scalar<real>("Top", m_configuration.m_camera_settings.top);
        invalidated |= imgui::input_scalar<real>("Bottom", m_configuration.m_camera_settings.bottom);
        invalidated |= imgui::input_scalar<real>("Near", m_configuration.m_camera_settings.near);
        invalidated |= imgui::input_scalar<real>("Far", m_configuration.m_camera_settings.far);
        ImGui::Text("Viewing Plane Rotation");
        rotation(m_configuration.m_camera_settings.rotation, "viewing_plane_rot_sliders");
        ImGui::Text("Ray Generation Rotation");
        rotation(m_configuration.m_camera_settings.ray_rotation, "ray_gen_rot_sliders");
    } else if (type == camera_type::orthographic) {
        invalidated |= imgui::input_scalar<real>("Physical Width", m_configuration.m_camera_settings.physical_width);
        ImGui::Text("Viewing Plane Rotation");
        rotation(m_configuration.m_camera_settings.rotation, "viewing_plane_rot_sliders");
        ImGui::Text("Ray Generation Rotation");
        rotation(m_configuration.m_camera_settings.ray_rotation, "ray_gen_rot_sliders");
    } else if (type == camera_type::frustum) {
        //
    } else {
        std::unreachable();
    }

    return invalidated;
}

void sfml_program::gui_render_settings() {
    static constexpr const char* integrator_names[]{
      "cosine-weighted albedo + emittance (useful for previews)",
      //"direct lighting checker (useful for previews)",
      "unidirectional path tracing",
    };

    static constexpr const char* camera_names[]{
      "pinhole",
      "environment",
      "orthographic (raw)",
      "frustum (raw)",
      "orthographic",
      "frustum",
    };

    ImGui::BeginGroup();
    //ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
    //ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.75f);

    // ImGui::SetNextItemWidth(64 * 2);
    imgui::input_scalar_n<unsigned>("Resolution", {&m_configuration.m_resolution.x, 2});

    ImGui::Checkbox("Render upon invalidation", &m_ui_render_on_invalidate);

    if (ImGui::Button("begin render")) {
        request_render();
    }

    if (m_ongoing_render.load(std::memory_order::relaxed)) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(255, 0, 0, 255), "Render in progress...");
    }

    if (ImGui::TreeNode("Integrator Settings")) {
        ImGui::Combo("Integrator", &reinterpret_cast<int&>(m_configuration.m_integrator), integrator_names, std::size(integrator_names));
        imgui::input_scalar("Samples per Pixel", m_configuration.m_integrator_settings.samples);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Camera Settings")) {
        m_invalidated |= ImGui::Combo("Camera Type", &reinterpret_cast<int&>(m_configuration.m_camera_settings.type), camera_names, std::size(camera_names));

        m_invalidated |= gui_render_camera_editor();
        m_invalidated |= imgui::input_scalar_n<real>("Camera Position", {m_configuration.m_camera_settings.center.data(), 3});
        // ImGui::SetNextItemWidth(64 * 3);

        if (ImGui::TreeNode("Movement")) {
            imgui::input_scalar("movement step size", m_ui_movement_step);

            enum class direction {
                east,
                north,
                west,
                south,
                up,
                down,
            };

            auto move = [this](direction dir) {

            };

            if (ImGui::BeginTable("movement_buttons", 3)) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Button("↗");
                ImGui::TableNextColumn();
                ImGui::Button("↑");
                ImGui::TableNextColumn();
                ImGui::Button("↘");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Button("←");
                ImGui::TableNextColumn();
                ImGui::Text(" ");
                ImGui::TableNextColumn();
                ImGui::Button("→");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Button("↷");
                ImGui::TableNextColumn();
                ImGui::Button("↓");
                ImGui::TableNextColumn();
                ImGui::Button("↶");

                ImGui::EndTable();
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    //ImGui::PopItemWidth();
    ImGui::EndGroup();
    return;
}

auto sfml_program::gui_render_albedo_source_editor(albedo_source& source) -> bool {
    auto guard = imgui::guarded_group();
    bool changed = false;

    const char* source_names[] = {
      "UV Visualiser",
      "Surface Normal Visualiser",
      "Color",
      "Texture (Not yet supported in the editor)",
    };

    stf::multi_visitor index_visitor{
      [](uv_albedo&) { return 0; },
      [](normal_albedo&) { return 1; },
      [](color&) { return 2; },
      [](texture&) { return 3; },
    };

    int current_source_index = std::visit(index_visitor, source);

    bool source_type_changed = ImGui::Combo("Color Source", &current_source_index, source_names, std::size(source_names));
    changed |= source_type_changed;

    if (source_type_changed) {
        switch (current_source_index) {
            case 0:
                source = uv_albedo{};
                break;
            case 1:
                source = normal_albedo{};
                break;
            case 2:
                source = color{};
                break;
            case 3:
                source = texture{};
                break;
            default:
                std::unreachable();
        }
    }

    stf::multi_visitor editor_visitor{
      [](uv_albedo& source) {//
          return imgui::input_scalar_n<real>("scale", {&source.scale[0], 3});
      },
      [](normal_albedo& source) {//
          return imgui::input_scalar_n<real>("scale", {&source.scale[0], 3});
      },
      [](color& source) {
          /*real scale = abs(source);
              color norm_color = source / scale;

              bool changed = false;

              float norm_color_f[3]{static_cast<float>(norm_color[0]), static_cast<float>(norm_color[1]), static_cast<float>(norm_color[2])};

              changed |= ImGui::ColorPicker3("color", norm_color_f);
              changed |= imgui::input_scalar("scale", scale);

              norm_color[0] = norm_color_f[0];
              norm_color[1] = norm_color_f[1];
              norm_color[2] = norm_color_f[2];

              source = norm_color * scale;*/

          return imgui::input_scalar_n<real>("color", {&source[0], 3});
      },
      [](texture& mat) {
          ImGui::Text("Cannot edit texture settings.");
          return false;
      },
    };

    changed |= std::visit(editor_visitor, source);

    return changed;
}

auto sfml_program::gui_render_material_editor() -> bool {
    if (m_soft_deleted_materials.contains(m_current_material_index) || m_current_material_index >= m_scene->m_materials.size()) {
        ImGui::Text("No material selected...");
        return false;
    }

    auto guard = imgui::guarded_group();

    material& cur_mat = m_scene->m_materials[m_current_material_index];

    bool changed = false;

    if (ImGui::TreeNode("Albedo Editor")) {
        changed |= gui_render_albedo_source_editor(VARIANT_MEMBER(cur_mat, m_albedo));
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Emittance Editor")) {
        changed |= gui_render_albedo_source_editor(VARIANT_MEMBER(cur_mat, m_emission));
        ImGui::TreePop();
    }

    stf::multi_visitor visitor{
      [](materials::fresnel_conductor& mat) {
          ImGui::Text("Fresnel conductors have no parameters.");
          return false;
      },
      [](materials::fresnel_dielectric& mat) {
          bool changed = false;
          changed |= imgui::input_scalar("Incoming medium refractivity index", mat.m_n_i);
          changed |= imgui::input_scalar("Transmission medium refractivity index", mat.m_n_t);
          return changed;
      },
      [](materials::lambertian& mat) {
          ImGui::Text("Lambertian surfaces have no parameters.");
          return false;
      },
      [](materials::oren_nayar& mat) {
          return imgui::input_scalar("Variance", mat.m_sigma);
      },
    };

    if (ImGui::TreeNode("Material Settings")) {
        auto guard = imgui::guarded_group();

        changed |= std::visit(visitor, cur_mat);

        ImGui::TreePop();
    }

    return changed;
}

auto sfml_program::gui_render_material_list() -> bool {
    if (ImGui::BeginTable("material_list_table", 6, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Model");
        ImGui::TableSetupColumn("Albedo Type");
        ImGui::TableSetupColumn(" ");
        ImGui::TableSetupColumn("Emittance Type");
        ImGui::TableSetupColumn(" ");
        ImGui::TableHeadersRow();

        for (u32 i = 0; auto const& material: m_scene->m_materials) {
            if (m_soft_deleted_materials.contains(i)) {
                ++i;
                continue;
            }

            ImGui::TableNextRow();

            /*ImGui::TableNextColumn();
                if (ImGui::Button("edit")) {
                    m_current_material_index = i;
                }*/

            stf::multi_visitor material_visitor{
              [](materials::fresnel_dielectric const&) -> const char* { return "Fresnel Dielectric"; },
              [](materials::fresnel_conductor const&) -> const char* { return "Fresnel Conductor"; },
              [](materials::lambertian const&) -> const char* { return "Lambertian"; },
              [](materials::oren_nayar const&) -> const char* { return "Oren-Nayar"; },
            };

            ImGui::TableNextColumn();

            if (ImGui::Selectable(fmt::format("{}", i).c_str(), i == m_current_material_index, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                if (ImGui::GetIO().KeyCtrl) {
                    m_soft_deleted_materials.emplace(i);
                } else {
                    m_current_material_index = i;
                }
            }

            ImGui::TableNextColumn();
            ImGui::Text("%s", std::visit(material_visitor, material));

            stf::multi_visitor albedo_visitor{
              [](uv_albedo const&) -> const char* { return "UV Visualiser"; },
              [](normal_albedo const&) -> const char* { return "Surface Normal Visualiser"; },
              [](color const&) -> const char* { return "Color"; },
              [](texture const&) -> const char* { return "Texture"; },
            };

            ImGui::TableNextColumn();
            ImGui::Text("%s", std::visit(albedo_visitor, VARIANT_MEMBER(material, m_albedo)));

            ImGui::TableNextColumn();

            ImGui::TableNextColumn();
            ImGui::Text("%s", std::visit(albedo_visitor, VARIANT_MEMBER(material, m_emission)));

            ImGui::TableNextColumn();

            i++;
        }
        ImGui::EndTable();

        if (ImGui::Button("Add New")) {
            m_scene->m_materials.emplace_back(materials::lambertian(color(0)));
        }
    }
    return false;
}

auto sfml_program::gui_render_object_editor() -> bool { return false; }

auto sfml_program::gui_render_scene_editor() -> bool { return false; }

}// namespace trc
