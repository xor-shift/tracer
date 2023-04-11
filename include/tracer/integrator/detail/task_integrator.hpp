#pragma once

#include <stuff/thread.hpp>

#include <tracer/integrator/integrator.hpp>

namespace trc::detail {

struct default_task {
    std::pair<usize, usize> xy_start;
    std::pair<usize, usize> span;
};

struct default_task_generator {
    using payload_type = default_task;

    constexpr default_task_generator() = default;

    constexpr default_task_generator(std::pair<usize, usize> chunk_size) noexcept
        : m_chunk_size(chunk_size) {}

    constexpr default_task_generator(default_task_generator const& other)
        : m_chunk_size(other.m_chunk_size)
        , m_image(other.m_image)
        , m_x_tasks(other.m_x_tasks)
        , m_y_tasks(other.m_y_tasks) {}

    constexpr void set_image(image_like& image) {
        m_x_tasks = image.width() / m_chunk_size.first + (image.width() % m_chunk_size.first != 0);
        m_y_tasks = image.height() / m_chunk_size.second + (image.height() % m_chunk_size.second != 0);
    }

    constexpr auto n_tasks() const -> usize {
        return m_x_tasks * m_y_tasks;
    }

    constexpr void reset() {
        m_next_task.store(0, std::memory_order::seq_cst);
    }

    constexpr auto next_task() -> std::optional<payload_type> {
        usize task_no = m_next_task.fetch_add(1, std::memory_order::seq_cst);
        if (task_no >= n_tasks()) {
            return std::nullopt;
        }

        return (*this)(task_no);
    }

    constexpr auto operator()(usize task) const -> payload_type {
        usize start_col = (task % m_x_tasks) * m_chunk_size.first;
        usize start_row = (task / m_x_tasks) * m_chunk_size.second;

        return default_task{
          .xy_start{start_col, start_row},
          .span{m_chunk_size},
        };
    }

private:
    std::pair<usize, usize> m_chunk_size{32, 32};

    image_view m_image{};
    usize m_x_tasks = 0;
    usize m_y_tasks = 0;

    std::atomic_size_t m_next_task = 0;
};

struct c4d_task_generator {
    using payload_type = default_task;

    constexpr c4d_task_generator() = default;

    constexpr c4d_task_generator(std::pair<usize, usize> chunk_size) noexcept
        : m_chunk_size(chunk_size) {}

    constexpr c4d_task_generator(c4d_task_generator const& other)
        : m_chunk_size(other.m_chunk_size)
        , m_image(other.m_image)
        , m_x_tasks(other.m_x_tasks)
        , m_y_tasks(other.m_y_tasks) {}

    constexpr void set_image(image_like& image) {
        m_x_tasks = image.width() / m_chunk_size.first + (image.width() % m_chunk_size.first != 0);
        m_y_tasks = image.height() / m_chunk_size.second + (image.height() % m_chunk_size.second != 0);
    }

    constexpr void reset() {}

    auto next_task() -> std::optional<payload_type> {
        isize half_x = m_x_tasks / 2;
        isize half_y = m_y_tasks / 2;

        std::unique_lock lock{m_task_gen_mutex};

        constexpr std::pair<isize, isize> dir_offsets[]{
          {1, 0},
          {0, -1},
          {-1, 0},
          {0, 1},
        };

        usize x_task;
        usize y_task;

        for (;;) {
            const auto [off_x, off_y] = dir_offsets[static_cast<int>(m_direction)];
            const auto current_offset = m_next_offset;
            m_next_offset.first += off_x;
            m_next_offset.second += off_y;

            advance();

            isize x_task_t = current_offset.first + half_x;
            isize y_task_t = current_offset.second + half_y;

            bool x_oob = x_task_t < 0 || x_task_t >= m_x_tasks;
            bool y_oob = y_task_t < 0 || y_task_t >= m_y_tasks;

            if (x_oob && y_oob) {
                return std::nullopt;
            }

            if (x_oob || y_oob) {
                continue;
            }

            x_task = static_cast<usize>(x_task_t);
            y_task = static_cast<usize>(y_task_t);
            break;
        }

        usize start_col = x_task * m_chunk_size.first;
        usize start_row = y_task * m_chunk_size.second;

        return default_task{
          .xy_start{start_col, start_row},
          .span{m_chunk_size},
        };
    }

private:
    std::pair<usize, usize> m_chunk_size{32, 32};

    image_view m_image{};
    isize m_x_tasks = 0;
    isize m_y_tasks = 0;

    enum class direction : int {
        east = 0,
        north = 1,
        west = 2,
        south = 3,
    };

    constexpr void advance() {
        if (++m_step_counter == m_steps) {
            m_step_counter = 0;
            m_direction = static_cast<direction>((static_cast<int>(m_direction) + 1) % 4);

            if (m_odd_direction_change) {
                m_steps++;
            }

            m_odd_direction_change ^= true;
        }
    }

    mutable std::mutex m_task_gen_mutex{};
    bool m_odd_direction_change = false;
    usize m_step_counter = 0;
    usize m_steps = 1;
    direction m_direction = direction::north;

    std::pair<isize, isize> m_next_offset{0, 0};
};

template<typename TaskGenerator = detail::c4d_task_generator>
struct task_integrator : integrator {
    using task_generator_type = TaskGenerator;
    using task_payload_type = typename TaskGenerator::payload_type;

    task_integrator(std::shared_ptr<camera> camera, std::shared_ptr<scene> scene, task_generator_type const& generator = {})
        : integrator(camera, scene)
        , m_task_generator(generator) {}

    constexpr void integrate(image_like& out, integration_settings opts, default_rng& gen) noexcept final override {
        m_task_generator.set_image(out);

        usize n_threads;

        if consteval {
            n_threads = 1;
        } else {
#ifdef NDEBUG
            n_threads = std::max(static_cast<int>(std::thread::hardware_concurrency()) - 1, 1);
#else
            n_threads = 1;
#endif
        }

        auto consume_task = [this, opts, &gen, &out] constexpr {
            auto task_opt = m_task_generator.next_task();
            if (!task_opt)
                return false;

            task_processor(*task_opt, out, opts, gen);
            return true;
        };

        if (n_threads <= 1) {
            for (;;) {
                if (!consume_task())
                    break;
            }

            return;
        }

        stf::wait_group wg{};
        std::vector<std::thread> workers{};

        m_task_generator.reset();

        auto worker = [this, consume_task, &wg, &out, opts](usize thread_id, usize stride, default_rng gen) {
            stf::scope::scope_exit wg_guard{[&wg] { wg.done(); }};

            for (;;) {
                if (!consume_task())
                    break;
            }
        };

        for (usize i = 0; i < n_threads; i++) {
            wg.add(1);

            default_rng thread_rng = gen;
            gen.jump_128();

            workers.emplace_back(worker, i, n_threads, thread_rng);
        }

        // this is not entirely necessary
        // keeping it just in case i switch away from std::thread in the future
        wg.wait();

        for (auto& w: workers) {
            w.join();
        }
    }

protected:
    virtual constexpr void task_processor(task_payload_type payload, image_like& out, integration_settings opts, default_rng& gen) noexcept = 0;

private:
    task_generator_type m_task_generator;
};

}// namespace trc::detail
