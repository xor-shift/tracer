#pragma once

#define TRACER_USING_SFML 1
#define IMGUI_DEFINE_MATH_OPERATORS

#include <tracer/run/program.hpp>

#include <stuff/thread/channel.hpp>

#include <tracer/camera.hpp>
#include <tracer/common.hpp>
#include <tracer/integrator/integrator.hpp>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

#include <imgui.h>

#include <memory>
#include <unordered_set>

namespace trc {

struct sfml_program final : program {
    sfml_program();

    ~sfml_program() final override;

    auto run() -> int final override;

private:
    enum class integrator_type : int {
        cosine_albedo = 0,
        //light_visibility = 1,
        unidirectional_pt = 1,
    };

    enum class camera_type : int {
        pinhole = 0,
        environment = 1,
        orthographic_raw = 2,
        frustum_raw = 3,
        orthographic = 4,
        frustum = 5,
    };

    struct camera_settings {
        camera_type type = camera_type::pinhole;

        // common

        vec3 center{};

        // pinhole, frustum

        real fov = 80;

        // pinhole, orthographic, frustum

        vec3 rotation{};

        // orthographic

        vec3 ray_rotation{};
        real physical_width = 5;

        // orthographic_raw and frustum_raw

        real left = -5;
        real right = 5;
        real bottom = -5;
        real top = 5;
        real near = 0;
        real far = 5;

        auto operator()(sf::Vector2u window_dimensions) const -> std::shared_ptr<camera>;
    };

    struct render_configuration {
        sf::Vector2u m_resolution;

        integrator_type m_integrator;
        integration_settings m_integrator_settings;

        camera_settings m_camera_settings;
    };

    render_configuration m_configuration{
      .m_resolution = sf::Vector2u{400, 300},
      .m_integrator = integrator_type::cosine_albedo,
      .m_integrator_settings = {
        .samples = 1,
      },
      .m_camera_settings = {},
    };

    std::shared_ptr<scene> m_scene = nullptr;
    u32 m_current_material_index = 0;
    std::unordered_set<u32> m_soft_deleted_materials{};

    sf::RenderWindow m_window;
    ImGuiContext* m_imgui_context = nullptr;
    std::unordered_map<std::string, ImFont*> m_fonts{};
    ImFont* m_font = nullptr;
    ImFontConfig m_font_config{};

    stf::qoi::image<> m_qoi_image{};
    image m_image{};
    std::array<char, 96> m_filename_buffer{};

    sf::Image m_sf_image{};
    std::mutex m_texture_mutex{};
    sf::Texture m_sf_texture{};

    stf::channel<render_configuration> m_render_request_channel{};
    stf::channel<void> m_render_complete_channel{};
    std::atomic_bool m_ongoing_render{};
    std::thread m_render_thread;

    // UI state
    real m_ui_movement_step = 1;
    bool m_ui_render_on_invalidate = true;

    bool m_invalidated = true;

    void load_fonts();

    auto recreate_images() -> bool;
    auto request_render() -> bool;
    void render_worker();

    void save_to_file(std::string_view filename);

    void gui_menu_bar();
    auto gui_render_camera_editor() -> bool;
    void gui_render_settings();
    auto gui_render_albedo_source_editor(albedo_source& source) -> bool;
    auto gui_render_material_editor() -> bool;
    auto gui_render_material_list() -> bool;
    auto gui_render_object_editor() -> bool;
    auto gui_render_scene_editor() -> bool;
};

}// namespace trc
