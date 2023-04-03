#include <stuff/ranvec.hpp>
#include <stuff/trilang.hpp>

#include <tracer/bvh/tree.hpp>
#include <tracer/camera/envrionment.hpp>
#include <tracer/camera/pinhole.hpp>
#include <tracer/colormap.hpp>
#include <tracer/common.hpp>
#include <tracer/integrator/unidirectional_pt.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <fstream>

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

void sfml_main(image_view image) {
    sf::Image sf_image{};
    sf::Texture texture{};
    sf::Sprite sprite{};

    sf_image.create({static_cast<u32>(image.width()), static_cast<u32>(image.height())});

    auto load_image = [&image, &sf_image, &texture, &sprite] {
        for (u32 row = 0; row < image.height(); row++) {
            for (u32 col = 0; col < image.width(); col++) {
                color c = srgb_oetf(image.at(col, row));

                sf::Color color{
                  static_cast<u8>(std::round(std::clamp(c[0], real(0), real(1)) * 255)),
                  static_cast<u8>(std::round(std::clamp(c[1], real(0), real(1)) * 255)),
                  static_cast<u8>(std::round(std::clamp(c[2], real(0), real(1)) * 255)),
                  //static_cast<u8>(std::round(std::clamp(c[3], real(0), real(1)) * 255)),
                  255,
                };
                sf_image.setPixel({col, row}, color);
            }
        }

        texture.loadFromImage(sf_image);
        sprite.setTexture(texture);
    };

    sf::RenderWindow window(sf::VideoMode(sf_image.getSize()), "tracer");
    window.setFramerateLimit(6);
    while (window.isOpen()) {
        for (sf::Event event; window.pollEvent(event);) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        load_image();

        window.clear();
        window.draw(sprite);
        window.display();
    }
}

constexpr auto get_scene_test() -> scene {
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
    u32 midx_mirror = PUSH_MATERIAL(materials::frensel_conductor(vec3(0.999), vec3(0)));
    u32 midx_glass = PUSH_MATERIAL(materials::frensel_dielectric(1, 1.5, vec3(0.999), vec3(0)));

#undef PUSH_MATERIAL

    real obj_radius = 0.85;
    vec3 left_obj_center{-1.3, -2.25 + obj_radius, 8.5};
    vec3 right_obj_center{1.3, -2.25 + obj_radius, 7.3};

    std::vector<bound_shape> bound_shapes{
      //shapes::box(midx_mirror, {left_obj_center - vec3(obj_radius), left_obj_center + vec3(obj_radius)}),
      shapes::sphere(midx_mirror, left_obj_center, obj_radius),
      shapes::sphere(midx_glass, right_obj_center, obj_radius),

      shapes::disc(midx_white_light, {0, 2.2499, 7.5}, {0, -1, 0}, 0.71),
      /*shapes::disc(midx_red_light, {-2, 2.2499, 7.5}, {0, -1, 0}, 0.71),
      shapes::disc(midx_green_light, {0, 2.2499, 7.5}, {0, -1, 0}, 0.71),
      shapes::disc(midx_blue_light, {2, 2.2499, 7.5}, {0, -1, 0}, 0.71),*/

      //shapes::box(midx_magdonal, {{-2.5, -2.25, 5}, {-0.5, -1.25, 7}}),

      //shapes::triangle(midx_green, {{left_obj_center + vec3{0, 0 - obj_radius, -2}, left_obj_center + vec3{1, 0 - obj_radius, -2}, left_obj_center + vec3{0, 1 - obj_radius, -2}}}),
    };

    std::vector<std::unique_ptr<dyn_shape>> dyn_shapes{};
    dyn_shapes.emplace_back(std::make_unique<bvh_tree>(std::move(bound_shapes)));

    std::vector<shape> shapes{
      shapes::plane(midx_red, {-2.8, 0, 10}, {1, 0, 0}),   // left
      shapes::plane(midx_uv0, {0, 0, 10}, {0, 0, -1}),     // back
      shapes::plane(midx_blue, {2.8, 0, 10}, {-1, 0, 0}),  // right
      shapes::plane(midx_white, {0, 2.25, 10}, {0, -1, 0}),// top
      shapes::plane(midx_white, {0, -2.25, 10}, {0, 1, 0}),// bottom
    };

    scene scene(std::move(shapes), std::move(materials), std::move(dyn_shapes));

    return scene;
}

static auto get_scene_teapot() -> scene {
    std::vector<material> materials{
      materials::lambertian(vec3(0), normal_albedo{}),
    };

    std::ifstream stl_file("Utah_teapot_(solid).stl");
    stf::trilang::stl::binary_stream triangle_stream{std::istreambuf_iterator<char>(stl_file), std::istreambuf_iterator<char>()};

    std::vector<bound_shape> teapot_triangles{};
    for (;;) {
        auto res = triangle_stream.next();
        if (!res) {
            break;
        }

        mat3x3 mat = rotation_matrix<real>(std::numbers::pi_v<real> * -90 / 180, 0, 0);

        vec3 v_0 = mat * vec3{res->vertices[0][0], res->vertices[0][1], res->vertices[0][2]};
        vec3 v_1 = mat * vec3{res->vertices[1][0], res->vertices[1][1], res->vertices[1][2]};
        vec3 v_2 = mat * vec3{res->vertices[2][0], res->vertices[2][1], res->vertices[2][2]};

        teapot_triangles.emplace_back(shapes::triangle(0, {v_0, v_1, v_2}));
    }

    std::vector<std::unique_ptr<dyn_shape>> dyn_shapes{};
    dyn_shapes.emplace_back(std::make_unique<bvh_tree>(std::move(teapot_triangles)));

    std::vector<shape> shapes{

    };

    return scene({}, std::move(materials), std::move(dyn_shapes));
}

int main() {
#ifdef NDEBUG
    usize samples = 64;
    usize width = 800;
    usize height = 600;
#else
    usize samples = 1;
    usize width = 400;
    usize height = 300;
#endif

    auto camera = std::make_shared<pinhole_camera>(vec3(0), vec2(width, height), 80. / 180. * std::numbers::pi_v<real>);
    //auto camera = std::make_shared<pinhole_camera>(vec3{0, 30, -15}, vec2(width, height), 80. / 180. * std::numbers::pi_v<real>, rotation_matrix<real>(std::numbers::pi_v<real> * 60 / 180, 0, 0));
    //auto camera = std::make_shared<environment_camera>(vec3(0), vec2(width, height));

    auto scene = std::make_shared<trc::scene>(get_scene_test());
    //auto scene = std::make_shared<trc::scene>(get_scene_teapot());

    unidirectional_pt integrator(std::move(camera), std::move(scene));

    image image(width, height);

    stf::random::xoshiro_256p gen{std::random_device{}()};

    integration_options opts{
      .samples = samples,
    };

    std::thread integrator_thread{[&] {
        integrator.integrate(trc::image_view{image}, opts, gen);

        stf::qoi::image qoi_image{};
        qoi_image.create(image.width(), image.height());

        for (usize y = 0; y < image.height(); y++) {
            for (usize x = 0; x < image.width(); x++) {
                color c = srgb_oetf(image.at(x, y));

                qoi_image.at(x, y) = stf::qoi::color{
                  static_cast<u8>(std::round(std::clamp(c[0], real(0), real(1)) * 255)),
                  static_cast<u8>(std::round(std::clamp(c[1], real(0), real(1)) * 255)),
                  static_cast<u8>(std::round(std::clamp(c[2], real(0), real(1)) * 255)),
                  255,
                };
            }
        }

        qoi_image.to_file("render.qoi");
    }};

    sfml_main(image);

    integrator_thread.join();

    return 0;
}
