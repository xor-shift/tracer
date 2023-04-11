#include <tracer/run/sfml/main.hpp>

/*
struct renderer {

private:
    render_configuration m_configuration{
      .resolution = sf::Vector2u{400, 300},
      .integrator = integrator_type::cosine_albedo,
      .integrator_settings = {
        .samples = 1,
      },
      .camera_settings = {
        .type = camera_type::pinhole,
        .fov = 80,
        .center{0, 0, 0},
        .rotation{0, 0, 0},
      },
    };

    std::shared_ptr<scene> m_scene = std::make_shared<scene>(std::move(get_scene_test()));
    u32 m_current_material_index = 0;
    std::unordered_set<u32> m_soft_deleted_materials{};

    sf::RenderWindow m_window;
    ImGuiContext* m_imgui_context = nullptr;
    std::unordered_map<std::string, ImFont*> m_fonts{};
    ImFont* m_font = nullptr;
    ImFontConfig m_font_config{};

    stf::qoi::image<> m_qoi_image{};
    image m_image{};
    std::array<char, 96> m_filename_buffer {};

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


};
*/

int main() {
    trc::sfml_program program{};
    return program.run();
}
