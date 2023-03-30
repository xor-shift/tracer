#include <stuff/ranvec.hpp>
#include <tracer/camera/envrionment.hpp>
#include <tracer/camera/pinhole.hpp>
#include <tracer/colormap.hpp>
#include <tracer/common.hpp>
#include <tracer/integrator.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

using namespace trc;

void sfml_main(image_view image) {
    auto srgb_oetf = [](color linear) -> color {
        real k = 0.0031308;
        auto cutoff = less_than(swizzle<"rgb">(linear), vec3(k));

        vec3 higher = vec3(1.055) * pow(swizzle<"rgb">(linear), vec3(1. / 2.4)) - vec3(.055);
        vec3 lower = swizzle<"rgb">(linear) * vec3(12.92);

        return mix(higher, lower, cutoff);
    };

    auto srgb_eotf = [](color srgb) -> color {
        auto cutoff = less_than(swizzle<"rgb">(srgb), vec3(0.04045));
        vec3 higher = pow((swizzle<"rgb">(srgb) + vec3(0.055)) / vec3(1.055), vec3(2.4));
        vec3 lower = swizzle<"rgb">(srgb) / vec3(12.92);

        return mix(higher, lower, cutoff);
    };

    sf::Image sf_image{};
    sf::Texture texture{};
    sf::Sprite sprite{};

    sf_image.create({static_cast<u32>(image.width()), static_cast<u32>(image.height())});

    auto load_image = [srgb_oetf, &image, &sf_image, &texture, &sprite] {
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

constexpr auto get_scene_smallpt() -> scene {
    std::vector<material> materials{};

    materials.reserve(32);// avoid reallocations for indices to be reliably gathered

#define PUSH_MATERIAL(...) std::distance(materials.data(), std::addressof(materials.emplace_back(__VA_ARGS__)))

    u32 midx_red = PUSH_MATERIAL(materials::lambertian(vec3{.75, .25, .25}, vec3(0)));
    u32 midx_blue = PUSH_MATERIAL(materials::lambertian(vec3{.25, .25, .75}, vec3(0)));
    u32 midx_white = PUSH_MATERIAL(materials::lambertian(vec3(.75), vec3(0)));
    u32 midx_black = PUSH_MATERIAL(materials::lambertian(vec3(0), vec3(0)));
    u32 midx_mirror = PUSH_MATERIAL(materials::frensel_conductor(vec3(1), vec3(0)));
    u32 midx_glass = PUSH_MATERIAL(materials::frensel_dielectric(1, 1.5, vec3(1), vec3(0)));
    u32 midx_light = PUSH_MATERIAL(materials::lambertian(vec3(0), vec3(12)));

#undef PUSH_MATERIAL

    std::vector<shape> shapes{
      shapes::sphere{{1e5 + 1, 40.8, 81.6}, 1e5, midx_red},
      shapes::sphere{{-1e5 + 99, 40.8, 81.6}, 1e5, midx_blue},
      shapes::sphere{{50, 40.8, 1e5}, 1e5, midx_white},
      shapes::sphere{{50, 40.8, -1e5 + 170}, 1e5, midx_black},
      shapes::sphere{{50, 1e5, 81.6}, 1e5, midx_white},
      shapes::sphere{{50, -1e5 + 81.6, 81.6}, 1e5, midx_white},
      shapes::sphere{{27, 16.5, 47}, 16.5, midx_mirror},
      shapes::sphere{{73, 16.5, 78}, 16.5, midx_glass},
      shapes::sphere{{50, 681.6 - .27, 81.6}, 600, midx_light},
    };

    scene scene(std::move(shapes), std::move(materials));

    return scene;
}

constexpr auto get_scene_test() -> scene {
    std::vector<material> materials{};

    materials.reserve(32);// avoid reallocations for indices to be reliably gathered

#define PUSH_MATERIAL(...) std::distance(materials.data(), std::addressof(materials.emplace_back(__VA_ARGS__)))

    u32 midx_red_on = PUSH_MATERIAL(materials::oren_nayar(20. / 180. * std::numbers::pi_v<real>, vec3{.75, .25, .25}, vec3(0)));
    u32 midx_blue_on = PUSH_MATERIAL(materials::oren_nayar(20. / 180. * std::numbers::pi_v<real>, vec3{.25, .25, .75}, vec3(0)));
    u32 midx_white_on = PUSH_MATERIAL(materials::oren_nayar(20. / 180. * std::numbers::pi_v<real>, vec3(.75), vec3(0)));
    u32 midx_red = PUSH_MATERIAL(materials::lambertian(vec3{.75, .25, .25}, vec3(0)));
    u32 midx_blue = PUSH_MATERIAL(materials::lambertian(vec3{.25, .25, .75}, vec3(0)));
    u32 midx_white = PUSH_MATERIAL(materials::lambertian(vec3(.75), vec3(0)));

    u32 midx_light = PUSH_MATERIAL(materials::lambertian(vec3(0), vec3(12)));
    u32 midx_uv0 = PUSH_MATERIAL(materials::lambertian(texture("uv_grid_0.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_uv1 = PUSH_MATERIAL(materials::lambertian(texture("uv_grid_1.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_uv2 = PUSH_MATERIAL(materials::lambertian(texture("uv_grid_2.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_surf = PUSH_MATERIAL(materials::lambertian(texture("kodim10.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_parrot = PUSH_MATERIAL(materials::lambertian(texture("kodim23.qoi", wrapping_mode::repeat, scaling_method::nearest), vec3(0)));
    u32 midx_mirror = PUSH_MATERIAL(materials::frensel_conductor(vec3(0.999), vec3(0)));
    u32 midx_glass = PUSH_MATERIAL(materials::frensel_dielectric(1, 2, vec3(0.999), vec3(0)));

#undef PUSH_MATERIAL

    std::vector<shape> shapes{
      shapes::plane{{-2.8, 0, 10}, {1, 0, 0}, midx_red},
      shapes::plane{{0, 0, 10}, {0, 0, -1}, midx_white},
      shapes::plane{{2.8, 0, 10}, {-1, 0, 0}, midx_blue},
      shapes::plane{{0, 2.25, 10}, {0, -1, 0}, midx_white},
      shapes::plane{{0, -2.25, 10}, {0, 1, 0}, midx_white},
      shapes::disc{{0, 2.2499, 7.5}, {0, -1, 0}, 0.71, midx_light},
      //shapes::sphere{{0, 10, 8}, 10 - 2.4, midx_light},
      shapes::sphere{{-1.3, -2.25 + 0.85, 8.5}, 0.85, midx_mirror},
      shapes::sphere{{1.3, -2.25 + 0.85, 7.3}, 0.85, midx_glass},
      shapes::disc{{0, 1, 9.5}, {0, 0, -1}, 1, midx_uv1},
    };

    scene scene(std::move(shapes), std::move(materials));

    return scene;
}

int main() {
#ifdef NDEBUG
    usize samples = 512;
    usize width = 800;
    usize height = 600;
#else
    usize samples = 1;
    usize width = 400;
    usize height = 300;
#endif

    auto camera = std::make_shared<pinhole_camera>(vec3(0), vec2(width, height), 80. / 180. * std::numbers::pi_v<real>);
    //auto camera = std::make_shared<environment_camera>(vec3(0), vec2(width, height));
    //auto camera = std::make_shared<pinhole_camera>(vec3(50,52,295.6), vec2(width, height), 90. / 180. * std::numbers::pi_v<real>, stf::blas::rotation_matrix<real, stf::blas::matrix>(0, 0, std::numbers::pi_v<real>));
    //auto camera = std::make_shared<environment_camera>(vec3(50,52,295.6), vec2(width, height));

    auto scene = std::make_shared<trc::scene>(get_scene_test());
    //auto scene = std::make_shared<trc::scene>(get_scene_smallpt());

    unidirectional_pt integrator(std::move(camera), std::move(scene));

    image image(width, height);

    stf::random::xoshiro_256p gen{std::random_device{}()};

    integration_options opts{
      .samples = samples,
    };

    std::thread integrator_thread { [&] {
        integrator.integrate(trc::image_view{image}, opts, gen);
    } };

    sfml_main(image);

    integrator_thread.join();

    return 0;
}
