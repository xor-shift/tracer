#include <stuff/ranvec.hpp>
#include <tracer/colormap.hpp>
#include <tracer/common.hpp>
#include <tracer/material/materials.hpp>
#include <tracer/ray.hpp>
#include <tracer/shape/shapes.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

using namespace trc;

void sfml_main(std::vector<std::vector<color>> const& image) {
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

    u32 width = image[0].size();
    u32 height = image.size();

    sf::Image sf_image{};
    sf::Texture texture{};
    sf::Sprite sprite{};

    sf_image.create({width, height});

    for (u32 row = 0; row < height; row++) {
        for (u32 col = 0; col < width; col++) {
            color c = image[row][col];

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

    sf::RenderWindow window(sf::VideoMode({width, height}), "tracer");
    window.setFramerateLimit(60);
    while (window.isOpen()) {
        for (sf::Event event; window.pollEvent(event);) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }
}

struct scene {
    constexpr scene(std::vector<shape>&& shapes, std::vector<material>&& materials)
        : m_materials(std::move(materials)) {
        for (shape& s: std::move(shapes)) {
            auto visitor = stf::multi_visitor{
              [this]<concepts::bound_shape T>(T&& s) { m_bound_shapes.emplace_back(std::move(s)); },
              [this]<concepts::unbound_shape T>(T&& s) { m_unbound_shapes.emplace_back(std::move(s)); },
            };
            std::visit(visitor, std::move(s));
        }
    }

    template<typename Fn>
    constexpr auto for_each_shape(Fn&& fn) const {
        for (bound_shape const& s: m_bound_shapes) {
            std::visit(std::forward<Fn>(fn), s);
        }

        for (unbound_shape const& s: m_unbound_shapes) {
            std::visit(std::forward<Fn>(fn), s);
        }
    }

    constexpr auto intersect(ray const& ray) const -> std::optional<intersection> {
        real best_t = std::numeric_limits<real>::infinity();
        std::optional<intersection> best_isection = std::nullopt;

        for_each_shape([&](concepts::shape auto const& shape) {
            std::optional<intersection> isection = shape.intersect(ray);
            if (!isection)
                return;

            if (best_t > isection->t) {
                best_t = isection->t;
                best_isection = isection;
            }
        });

        return best_isection;
    }

    constexpr auto materials() const -> std::span<const material> {
        return {m_materials};
    }

    template<typename Gen>
    constexpr auto pick_light(Gen& gen) const -> shapes::disc {
        shapes::disc ret;
        std::uniform_int_distribution<usize> dist(0, m_bound_shapes.size() - 1);

        do {
            for (;;) {
                auto temp = m_bound_shapes[dist(gen)];
                if (std::holds_alternative<shapes::disc>(temp)) {
                    ret = std::get<shapes::disc>(temp);
                    break;
                }
            }
        } while (!is_light(m_materials[ret.material_index]));

        return ret;
    }

    constexpr auto visibility_check(vec3 a, vec3 b) const -> bool {
        real dist = abs(b - a);
        vec3 dir = (b - a) / dist;
        ray test_ray(a + dir * 0.0001, dir);

        intersection isection;
        if (auto isection_res = intersect(test_ray); !isection_res) {
            return true;
        } else {
            isection = *isection_res;
        }

        auto t = isection.t + 0.01 >= dist;

        if (!t) {
            std::ignore = std::ignore;
        }

        return t;
    }

private:
    std::vector<material> m_materials;

    std::vector<bound_shape> m_bound_shapes{};
    std::vector<unbound_shape> m_unbound_shapes{};

    static constexpr shapes::sphere s_stub_shape{};
};

struct pinhole_camera {
    constexpr pinhole_camera() = default;

    constexpr pinhole_camera(vec2 dimensions, real fov) {
        init(dimensions, fov);
    }

    constexpr void init(vec2 dimensions, real fov) {
        real theta = fov / 2;
        m_d = (1 / (2 * std::sin(theta))) * std::sqrt(std::abs(dimensions[0] * (2 - dimensions[0])));
    }

    constexpr auto generate_ray(vec3 origin, vec2 pixel) const -> ray {
        return ray(origin, normalize(vec3(pixel, m_d)));
    }

private:
    real m_d = 0;
};

template<typename Camera, typename Gen>
constexpr auto kernel_light_visibility(vec2 pixel, scene const& scene, Camera const& camera, Gen& gen) -> color {
    ray ray = camera.generate_ray(vec3(0), pixel);

    intersection isect;
    if (std::optional<intersection> isect_res = scene.intersect(ray); !isect_res) {
        return color{0, 0, 0};
    } else {
        isect = *isect_res;
    }

    concepts::bound_shape auto picked_shape = scene.pick_light(gen);

    usize hits = 0;
    usize tries = 4;

    for (usize i = 0; i < tries; i++) {
        intersection sample = picked_shape.sample_surface(gen);

        hits += scene.visibility_check(isect.isection_point + isect.normal * 0.0001, sample.isection_point + sample.normal * 0.0001);
    }

    return color(1) * static_cast<real>(hits) / static_cast<real>(tries);
}

template<typename Gen>
constexpr auto trace(scene const& scene, Gen& gen, ray const& ray, usize depth = 0) -> color {
    intersection isect;
    if (std::optional<intersection> isect_res = scene.intersect(ray); !isect_res) {
        return color{0, 0, 0};
    } else {
        isect = *isect_res;
    }

    auto mat_sample = sample(scene.materials()[isect.material_index], isect, gen);

    vec3 wi = stf::random::sphere_sampler<2>::sample<real>(gen);
    if (dot(wi, isect.normal) < 0) {
        wi = -wi;
    }

    real prob = 0.5 * std::numbers::pi_v<real>;
    real pdf = 0.5 * std::numbers::pi_v<real>;

    stf::random::erand48_distribution<real> dist{};
    real v = dist(gen);

    if (depth <= 4 || v >= .75) {
        trc::ray new_ray(isect.isection_point + wi * 0.001, wi);
        real cos_weight = dot(wi, isect.normal);
        real weight = cos_weight * pdf / prob;
        color attenuation = weight * mat_sample.albedo;

        return mat_sample.emittance + attenuation * trace(scene, gen, new_ray, depth + 1);
    } else {
        return mat_sample.emittance;
    }
}

template<typename Camera, typename Gen>
constexpr auto kernel_recursive_pt(vec2 pixel, scene const& scene, Camera const& camera, Gen& gen) -> color {
    ray ray = camera.generate_ray(vec3(0), pixel);

    return trace(scene, gen, ray);
}

template<typename Gen>
constexpr auto generate_backward_path(ray ray, std::span<interaction> out, scene const& scene, Gen& gen) -> std::span<interaction> {
    usize i;

    for (i = 0; i < out.size(); i++) {
        auto isect_res = scene.intersect(ray);
        if (!isect_res)
            break;

        intersection isect = *isect_res;

        auto interaction = sample(scene.materials()[isect.material_index], isect, gen);

        out[i] = interaction;

        stf::random::erand48_distribution<real> dist{};
        if (i >= out.size() / 2 && dist(gen) > .75) {
            break;
        }

        ray = {isect.isection_point + interaction.wi * 0.001, interaction.wi};
    }

    return std::span(out.data(), i);
}

constexpr auto compute_backward_path(std::span<interaction> path) -> color {
    color out_color = color(0);
    color cur_attenuation = color(1);
    for (usize i = 0; i < path.size(); i++) {
        auto const& [wi, pdf, albedo, emittance, isect] = path[i];

        real wi_probability = 0.5 * std::numbers::pi_v<real>;

        out_color = out_color + emittance * cur_attenuation;

        real cos_weight = dot(wi, isect.normal);
        real weight = cos_weight * pdf / wi_probability;
        color attenuation = weight * albedo;
        cur_attenuation = cur_attenuation * attenuation;
    }

    return out_color;
}

template<typename Camera, typename Gen>
constexpr auto kernel_iterative_pt(vec2 pixel, scene const& scene, Camera const& camera, Gen& gen) -> color {
    std::array<interaction, 8> path_storage;
    std::span<interaction> path = generate_backward_path(camera.generate_ray(vec3(0), pixel), path_storage, scene, gen);

    return compute_backward_path(path);
}

template<typename Gen>
constexpr auto generate_forward_path(concepts::bound_shape auto const& shape, std::span<interaction> out, scene const& scene, Gen& gen) -> std::span<interaction> {
    intersection initial_isection = shape.sample_surface(gen);
    vec3 initial_wo = stf::random::sphere_sampler<2>::sample<real>(gen);
    if (dot(initial_isection.isection_point - shape.center(), initial_isection.normal) < 0)
        initial_wo = -initial_wo;
    initial_isection.wo = initial_wo;

    interaction initial_interaction = sample(scene.materials()[initial_isection.material_index], initial_isection, gen);
    initial_interaction.pdf = 0;

    ray initial_ray(initial_isection.isection_point + initial_wo * 0.0001, initial_wo);

    out[0] = initial_interaction;
    usize path_size = generate_backward_path(initial_ray, out.subspan(1), scene, gen).size() + 1;

    auto ret = std::span(out.data(), path_size);

    for (interaction& interaction: ret) {
        using std::swap;
        swap(interaction.wi, interaction.isection.wo);
    }

    return ret;
}

template<typename Camera, typename Gen>
constexpr auto kernel_light_trace(vec2 pixel, scene const& scene, Camera const& camera, Gen& gen) -> color {
    auto light_source = scene.pick_light(gen);

    std::array<interaction, 7> forward_path_storage;
    std::span<interaction> forward_path = generate_forward_path(light_source, std::span(forward_path_storage), scene, gen);
    vec3 center(0);

    ray camera_ray = camera.generate_ray(center, pixel);

    intersection camera_intersection;
    if (auto res = scene.intersect(camera_ray); !res) {
        return color(0);
    } else {
        camera_intersection = *res;
    }
    interaction camera_interaction = sample(scene.materials()[camera_intersection.material_index], camera_intersection, gen);

    if (!scene.visibility_check(camera_intersection.isection_point + camera_intersection.normal * 0.0001, forward_path.back().isection.isection_point)) {
        return color(0);
    }

    interaction& light_end = forward_path.back();

    camera_interaction.wi = normalize(light_end.isection.isection_point - camera_interaction.isection.isection_point);
    camera_interaction.pdf = brdf(scene.materials()[camera_interaction.isection.material_index], camera_interaction.isection.isection_point, camera_interaction.wi, camera_interaction.isection.wo);
    light_end.isection.wo = -camera_interaction.wi;
    light_end.pdf = brdf(scene.materials()[light_end.isection.material_index], light_end.isection.isection_point, light_end.wi, light_end.isection.wo);

    color flux(0);
    for (interaction const& interaction: forward_path) {
        real wi_probability = 0.5 * std::numbers::pi_v<real>;

        auto const& [wi, pdf, albedo, emittance, isect] = interaction;

        real cos_weight = dot(wi, isect.normal);
        real weight = cos_weight * pdf / wi_probability;
        color attenuation = weight * albedo;

        flux = flux * attenuation;
        flux = flux + interaction.emittance;
    }

    interaction last_light_interaction = light_end;
    last_light_interaction.emittance = last_light_interaction.emittance + flux;

    std::array<interaction, 2> backward_path{{
      camera_interaction,
      last_light_interaction,
    }};

    return compute_backward_path(backward_path);

    /*vec3 camera_wi = normalize(forward_path.back().isection.isection_point - center);
    vec3 camera_normal = supposed_camera_ray.direction;
    real camera_pdf = 0.5 * std::numbers::pi_v<real>;
    real camera_wi_prob = 0.5 * std::numbers::pi_v<real>;
    real camera_weight = dot(camera_wi, camera_normal) * camera_pdf / camera_wi_prob;

    return flux * std::pow(dot(camera_wi, camera_normal), 32);*/
}

constexpr auto meld_paths(scene const& scene, interaction& from, interaction& to) -> real {
    from.wi = normalize(to.isection.isection_point - from.isection.isection_point);
    //to.isection.t = abs(to.isection.isection_point - from.isection.isection_point);
    to.isection.wo = -from.wi;

    from.pdf = brdf(scene.materials()[from.isection.material_index], from.isection.isection_point, from.wi, from.isection.wo);
    to.pdf = brdf(scene.materials()[to.isection.material_index], to.isection.isection_point, to.wi, to.isection.wo);

    // geometry term
    return 1.;
}

template<typename Camera, typename Gen>
constexpr auto kernel_bdpt(vec2 pixel, scene const& scene, Camera const& camera, Gen& gen) -> color {
    auto light_source = scene.pick_light(gen);

    std::array<interaction, 4> forward_path_storage;
    std::span<interaction> forward_path = generate_forward_path(light_source, std::span(forward_path_storage), scene, gen);
    std::array<interaction, 4> backward_path_storage;
    std::span<interaction> backward_path = generate_backward_path(camera.generate_ray(vec3(0), pixel), std::span(backward_path_storage), scene, gen);

    std::reverse(forward_path.begin(), forward_path.end());

    if (!scene.visibility_check(backward_path.back().isection.isection_point, forward_path.front().isection.isection_point)) {
        return color(0);
    }

    meld_paths(scene, backward_path.back(), forward_path.front());

    std::array<interaction, 8> path_storage {};

    auto it = std::copy(backward_path.begin(), backward_path.end(), path_storage.begin());
    it = std::copy(forward_path.begin(), forward_path.end(), it);

    std::span<interaction> path(path_storage.begin(), it);

    return compute_backward_path(path);
}

constexpr auto get_scene_uv_test() -> scene {
    std::vector<shape> shapes{
      shapes::sphere{
        .center_point = {-1, 1.5, 6},
        .radius = 0.5,
        .material_index = 0,
      },
      shapes::sphere{
        .center_point = {0, 1.5, 5},
        .radius = 0.5,
        .material_index = 1,
      },
      shapes::sphere{
        .center_point = {1, 1.5, 6},
        .radius = 0.5,
        .material_index = 2,
      },
      shapes::plane{
        .center = {0, 0, 0},
        .normal = {0, 1, 0},
        .material_index = 6,
      },
      shapes::disc{
        .center_point = {0, 2.5, 5},
        .normal = normalize(vec3(0, 1, -1)),
        .radius = 0.3,
        .material_index = 8,
      },
    };

    std::vector<material> materials{
      materials::texture{
        .texture = texture("uv_grid_0.qoi", wrapping_mode::clamp, scaling_method::nearest),
        .emissive = false,
      },
      materials::texture{
        .texture = texture("uv_grid_1.qoi", wrapping_mode::clamp, scaling_method::nearest),
        .emissive = false,
      },
      materials::texture{
        .texture = texture("uv_grid_2.qoi", wrapping_mode::clamp, scaling_method::nearest),
        .emissive = false,
      },
      materials::simple_color{
        .color = {0.5, 0, 0, 1},
        .emissive = false,
      },
      materials::simple_color{
        .color = {0, 0.5, 0, 1},
        .emissive = false,
      },
      materials::simple_color{
        .color = {0, 0, 0.5, 1},
        .emissive = false,
      },
      materials::texture{
        .texture = texture("testcard.qoi", wrapping_mode::repeat, scaling_method::nearest),
        .emissive = false,
      },
      materials::texture{
        .texture = texture("kodim10.qoi", wrapping_mode::clamp, scaling_method::nearest),
        .emissive = false,
      },
      materials::texture{
        .texture = texture("kodim23.qoi", wrapping_mode::clamp, scaling_method::nearest),
        .emissive = false,
      },
    };

    scene scene(std::move(shapes), std::move(materials));

    return scene;
}

constexpr auto get_scene_smallpt() -> scene {
    std::vector<shape> shapes{
      shapes::plane{
        // left wall, red
        .center{-2.5, 0, 10},
        .normal{1, 0, 0},
        .material_index = 0,
      },
      shapes::plane{
        // front wall, white
        .center{0, 0, 10},
        .normal{0, 0, -1},
        .material_index = 2,
      },
      shapes::plane{
        // right wall, blue
        .center{2.5, 0, 10},
        .normal{-1, 0, 0},
        .material_index = 1,
      },
      shapes::plane{
        // top wall, white
        .center{0, 2.5, 10},
        .normal{0, -1, 0},
        .material_index = 2,
      },
      shapes::plane{
        // bottom wall, white
        .center{0, -2.5, 10},
        .normal{0, 1, 0},
        .material_index = 2,
      },
      shapes::disc{
        .center_point{0, 2.475, 8},
        .normal{0, -1, 0},
        .radius = 1,
        .material_index = 3,
      },
      shapes::sphere{
        .center_point{0, 0, 9},
        // .normal{0, 0, -1},
        .radius = 1,
        .material_index = 8,
      },
      /*shapes::sphere{
        .center_point{0, 2.475, 8},
        .radius = 2,
        .material_index = 0,
      },*/
    };

    std::vector<material> materials{
      materials::simple_color{
        .color = {.75, .25, .25},
        .emissive = false,
      },
      materials::simple_color{
        .color = {.25, .25, .75},
        .emissive = false,
      },
      materials::simple_color{
        .color = {.75, .75, .75},
        .emissive = false,
      },
      materials::simple_color{
        .color = {12, 12, 12},
        .emissive = true,
      },
      materials::texture{
        .texture = texture("uv_grid_0.qoi", wrapping_mode::repeat, scaling_method::nearest),
        .emissive = false,
      },
      materials::texture{
        .texture = texture("uv_grid_1.qoi", wrapping_mode::repeat, scaling_method::nearest),
        .emissive = false,
      },
      materials::texture{
        .texture = texture("uv_grid_2.qoi", wrapping_mode::repeat, scaling_method::nearest),
        .emissive = false,
      },
      materials::texture{
        .texture = texture("kodim10.qoi", wrapping_mode::repeat, scaling_method::nearest),
        .emissive = false,
      },
      materials::texture{
        .texture = texture("kodim23.qoi", wrapping_mode::repeat, scaling_method::nearest),
        .emissive = false,
      },
    };

    scene scene(std::move(shapes), std::move(materials));

    return scene;
}

int main() {
#ifdef NDEBUG
    usize samples = 1024 * 2;
    isize width = 400;
    isize height = 300;
#else
    usize samples = 1;
    isize width = 400;
    isize height = 300;
#endif

    vec2 dims(width, height);

    scene scene = get_scene_smallpt();
    pinhole_camera camera(vec2(width, height), 90. / 180. * std::numbers::pi_v<real>);

    std::vector<std::vector<color>> image(height, std::vector(width, color{}));

    stf::random::xoshiro_256p gen{std::random_device{}()};
    stf::random::erand48_distribution<real> dist{};

#pragma omp parallel for
    for (isize row = 0; row < height; row++) {
        stf::random::xoshiro_256p gen_local{std::random_device{}()};

        for (isize col = 0; col < width; col++) {
            isize flipped_row = height - row - 1;
            vec2 cr_start = vec2(col, flipped_row) - (dims / 2);

            color sum{};

            for (usize i = 0; i < samples; i++) {
                vec2 sample = vec2(dist(gen_local), dist(gen_local));

                // sum = sum + kernel_iterative_pt(cr_start + sample, scene, camera, gen_local);
                sum = sum + kernel_bdpt(cr_start + sample, scene, camera, gen_local);
            }

            image[row][col] = sum / samples;
        }
    }

    sfml_main(image);

    return 0;
}
