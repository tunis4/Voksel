#include "game.hpp"
#include "renderer/renderer.hpp"
#include "renderer/chunk_renderer.hpp"
#include "renderer/sky_renderer.hpp"
#include "renderer/box_renderer.hpp"
#include "renderer/sorter.hpp"
#include "renderer/texture.hpp"
#include "renderer/imgui_impl_voksel.hpp"
#include "player.hpp"

#include <chrono>
#include <GLFW/glfw3.h>
#include <entt/entt.hpp>
#include <imgui/imgui_impl_glfw.h>

static Game *game_singleton = nullptr;

struct Movement {
    vec3 pos, old_pos, visual_pos, velocity;
    std::chrono::steady_clock::time_point pos_time;

    Movement(vec3 pos) : pos(pos), old_pos(pos), visual_pos(pos), velocity(0.0), pos_time(std::chrono::steady_clock::now()) {}
};

Game::Game(bool in_game) {
    game_singleton = this;

    m_window = new Window(1280, 720, "Voksel");
    m_window->enable_cursor();

    m_window->set_framebuffer_size_callback([&] (uint width, uint height) {
        entt::locator<renderer::Renderer>::value().on_framebuffer_resize(width, height);
    });

    m_window->set_cursor_pos_callback([&] (f64 x, f64 y) {
        if (!m_window->m_cursor_enabled) {
            m_camera->process_mouse_movement(x, y);
            entt::locator<renderer::Renderer>::value().on_cursor_move(x, y);
        } else {
            m_window->m_cursor_x = x;
            m_window->m_cursor_y = y;
        }
    });

    auto &renderer = entt::locator<renderer::Renderer>::emplace();
    renderer.init();

    m_main_thread = std::thread([this, in_game] {
        register_blocks();

        if (in_game)
            enter_singleplayer();
        
        m_running = true;
        heartbeat();
    });
}

Game::~Game() {
    m_running = false;
    m_main_thread.join();
    auto &renderer = entt::locator<renderer::Renderer>::value();
    renderer.begin_cleanup();
    if (m_in_game) {
        entt::locator<renderer::Sorter>::value().cleanup();
        entt::locator<renderer::BoxRenderer>::value().cleanup();
        entt::locator<renderer::SkyRenderer>::value().cleanup();
        entt::locator<renderer::ChunkRenderer>::value().cleanup();
    }
    renderer.cleanup();
    if (m_in_game)
        delete m_camera;
    delete m_window;
    game_singleton = nullptr;
}

Game* Game::get() {
    return game_singleton;
}

void Game::enter_singleplayer() {
    if (m_in_game)
        return;
    auto *world = new World();
    m_world = world;

    m_local_player = m_registry.create();
    m_registry.emplace<Movement>(m_local_player, vec3(16.5, 24.5, 16.5));

    m_in_game = true;
    m_camera = new Camera(vec3(0.0));
    m_camera->set_free(false);
    auto &renderer = entt::locator<renderer::Renderer>::value();
    entt::locator<renderer::ChunkRenderer>::emplace(renderer.m_context).init();
    entt::locator<renderer::SkyRenderer>::emplace(renderer.m_context).init();
    entt::locator<renderer::BoxRenderer>::emplace(renderer.m_context).init();
    entt::locator<renderer::Sorter>::emplace(renderer.m_context).init();
}

void Game::exit_singleplayer() {
    if (!m_in_game)
        return;
    m_in_game = false;
    vkDeviceWaitIdle(entt::locator<renderer::Renderer>::value().m_context.device);
    entt::locator<renderer::Sorter>::value().cleanup();
    entt::locator<renderer::BoxRenderer>::value().cleanup();
    entt::locator<renderer::SkyRenderer>::value().cleanup();
    entt::locator<renderer::ChunkRenderer>::value().cleanup();
    if (m_camera) {
        delete m_camera;
        m_camera = nullptr;
    }
    if (m_world) {
        delete m_world;
        m_world = nullptr;
    }
}

static f32 gravitational_acceleration = 16.0f;
static vec3 movement_velocity = vec3(0);
static std::mutex movement_velocity_mutex;

static bool override_camera_speed = false;
static bool unlock_block_modification = false;

static f32 target_fov = Camera::default_fov;

static constexpr f64 period = 1.0 / 60.0;

void Game::heartbeat() {
    std::chrono::steady_clock::time_point a = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point b = std::chrono::steady_clock::now();
    while (m_running) {
        a = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::ratio<1, 1>> work_time = a - b;

        if (work_time.count() < period) {
            auto delta_ms = std::chrono::duration_cast<std::chrono::duration<i64, std::milli>>(std::chrono::duration<f64>(period) - work_time);
            std::this_thread::sleep_for(delta_ms + std::chrono::duration<f64, std::milli>(0.5));
        }

        b = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::ratio<1, 1>> sleep_time = b - a;

        tick();
        m_tick_count++;
        m_tick_delta_time = (work_time + sleep_time).count();

        // log(LogLevel::INFO, "Heartbeat", "Work time: {} ms, Sleep time: {} ms, TPS: {}", work_time.count() * 1000.0, sleep_time.count() * 1000.0, 1.0 / m_tick_delta_time);
    }
}

void Game::tick() {
    if (m_in_game) {
        m_world->shape_chunks();
        m_world->decorate_chunks();
        m_world->apply_changes();
        m_world->mesh_chunks();
        m_world->check_chunks();

        {
            std::scoped_lock guard(movement_velocity_mutex);
            auto &movement = m_registry.get<Movement>(m_local_player);
            movement.velocity.x = movement_velocity.x;
            movement.velocity.z = movement_velocity.z;
            movement.velocity.y += movement_velocity.y;
        }

        auto view = m_registry.view<Movement>();
        for (auto entity : view) {
            auto &movement = view.get<Movement>(entity);

            movement.velocity.y -= gravitational_acceleration * m_tick_delta_time;
            if (movement.velocity.y < -55.5f)
                movement.velocity.y = -55.5f;
            
            movement.old_pos = movement.pos;
            movement.pos = resolve_movement(m_world, movement.pos, movement.pos + movement.velocity * m_tick_delta_time, vec3(-0.3, -1.5, -0.3), vec3(0.3, 0.3, 0.3), &movement.velocity);
            movement.pos_time = std::chrono::steady_clock::now();
        }

        m_camera->m_ray_cast = m_world->cast_ray(m_camera->pos(), m_camera->pos() + m_camera->m_front * 10.0f);
    }
}

void Game::render() {
    while (!m_window->should_close()) {
        auto frame_start = std::chrono::steady_clock::now();
        m_frame_count++;

        ImGui_ImplVoksel_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        entt::locator<renderer::TextureManager>::value().create_pending_textures();

        static bool show_demo_window = false;
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        static bool exiting_singleplayer = false;
        if (exiting_singleplayer) {
            exit_singleplayer();
            exiting_singleplayer = false;
        }

        auto &renderer = entt::locator<renderer::Renderer>::value();
        if (m_in_game) {
            if (!m_window->m_cursor_enabled) {
                ImGui::Begin("Crosshair", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
                auto draw = ImGui::GetBackgroundDrawList();
                draw->AddCircleFilled(ImVec2(m_window->m_width / 2.0f, m_window->m_height / 2.0f), 2.0f, IM_COL32(255, 255, 255, 255));
                ImGui::End();
            }

            ImGui::Begin("voksel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            if (ImGui::Button("Open ImGui demo window"))
                show_demo_window = !show_demo_window;

            if (ImGui::Button("Exit to main menu"))
                exiting_singleplayer = true;

            ImGui::Text("Press ALT to toggle cursor grab");
            ImGui::Text("Press F to toggle free cam");
            static bool show_utilities_window = false;
            if (ImGui::Button("Show utilities window"))
                show_utilities_window = !show_utilities_window;

            ImGui::Text("Block position: %d, %d, %d", m_camera->block_pos().x, m_camera->block_pos().y, m_camera->block_pos().z);
            auto [chunk_pos, chunk_offset] = signed_i32vec3_divide(m_camera->block_pos(), Chunk::size);
            ImGui::Text("Chunk position: %d, %d, %d", chunk_pos.x, chunk_pos.y, chunk_pos.z);
            ImGui::Text("Chunk offset: %d, %d, %d", chunk_offset.x, chunk_offset.y, chunk_offset.z);
            ImGui::Text("Position: %f, %f, %f", m_camera->pos().x, m_camera->pos().y, m_camera->pos().z);

            if (show_utilities_window) {
                ImGui::Begin("Utilities", &show_utilities_window);
                ImGui::Checkbox("Unlock block modification", &unlock_block_modification);
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.5f);
                ImGui::SliderFloat("Gravity", &gravitational_acceleration, -16, 32);
                static i32 coords[3] = {};
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.5f);
                ImGui::InputInt3("Coordinates", coords);
                if (ImGui::Button("Teleport")) {
                    m_registry.get<Movement>(m_local_player).pos = vec3(coords[0], coords[1], coords[2]);
                }
                if (m_camera->is_free()) {
                    ImGui::Checkbox("Override camera speed", &override_camera_speed);
                    if (override_camera_speed) {
                        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.5f);
                        ImGui::SliderFloat("Camera speed", &m_camera->m_free_speed, 5, 100);
                    }
                }
                ImGui::End();
            }

            // ImGui::Text("Velocity: %f, %f, %f", frame_velocity.x, frame_velocity.y, frame_velocity.z);
            
            auto view = m_registry.view<Movement>();
            for (auto entity : view) {
                auto &movement = view.get<Movement>(entity);
                std::chrono::duration<double, std::ratio<1, 1>> interpolation_time = std::chrono::steady_clock::now() - movement.pos_time;
                movement.visual_pos = glm::mix(movement.old_pos, movement.pos, interpolation_time.count() / period);
            }
            m_camera->m_pos = m_registry.get<Movement>(m_local_player).visual_pos;

            process_input();
            m_camera->m_fov = std::lerp(m_camera->m_fov, target_fov, 0.25f * 60.0f * m_render_delta_time);
            m_camera->update_matrices(m_window->m_width, m_window->m_height);
            m_camera->m_frustum.update(m_camera->m_unreversed_projection_matrix * m_camera->m_view_matrix);

            if (m_camera->m_ray_cast.hit) {
                i32vec3 block_pos = m_camera->m_ray_cast.block_pos;
                ImGui::Text("Looking at: %d, %d, %d  ID: %u", block_pos.x, block_pos.y, block_pos.z, m_world->get_block_at(block_pos));
                entt::locator<renderer::BoxRenderer>::value().set_box(true, vec3(block_pos));
            } else {
                entt::locator<renderer::BoxRenderer>::value().set_box(false);
            }

            entt::locator<renderer::ChunkRenderer>::value().update(m_render_delta_time);
        } else {
            ImGui::Begin("voksel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            // if (ImGui::Button("Open ImGui demo window"))
            //     show_demo_window = !show_demo_window;

            if (ImGui::Button("Singleplayer"))
                enter_singleplayer();

            if (ImGui::Button("Quit"))
                m_window->close();
        }

        renderer.render(m_render_delta_time);

        glfwPollEvents();

        auto frame_end = std::chrono::steady_clock::now();
        m_render_delta_time = std::chrono::duration_cast<std::chrono::duration<f32>>(frame_end - frame_start).count(); // cast to seconds as f32
    }
}

void Game::process_input() {
    if (m_window->is_key_pressed(GLFW_KEY_ESCAPE))
        m_window->close();

    static bool alt_locked = false;
    if (m_window->is_key_pressed(GLFW_KEY_LEFT_ALT)) {
        if (!alt_locked) {
            m_window->toggle_cursor();
            alt_locked = true;
        }
    } else alt_locked = false;

    static bool f_locked = false;
    if (m_window->is_key_pressed(GLFW_KEY_F)) {
        if (!f_locked) {
            m_camera->set_free(!m_camera->is_free());
            f_locked = true;
        }
    } else f_locked = false;

    static BlockNID block_to_place = 5;
    if (m_window->is_key_pressed(GLFW_KEY_1)) block_to_place = 1;
    if (m_window->is_key_pressed(GLFW_KEY_2)) block_to_place = 2;
    if (m_window->is_key_pressed(GLFW_KEY_3)) block_to_place = 3;
    if (m_window->is_key_pressed(GLFW_KEY_4)) block_to_place = 4;
    if (m_window->is_key_pressed(GLFW_KEY_5)) block_to_place = 5;
    if (m_window->is_key_pressed(GLFW_KEY_6)) block_to_place = 6;
    if (m_window->is_key_pressed(GLFW_KEY_7)) block_to_place = 7;
    if (m_window->is_key_pressed(GLFW_KEY_8)) block_to_place = 8;
    if (m_window->is_key_pressed(GLFW_KEY_9)) block_to_place = 9;
    if (m_window->is_key_pressed(GLFW_KEY_0)) block_to_place = 10;

    std::optional<i32vec3> changed_block_pos;
    static bool left_mouse_last = GLFW_RELEASE;
    static auto last_break = std::chrono::steady_clock::now();
    if (m_window->is_mouse_button_pressed(GLFW_MOUSE_BUTTON_LEFT)) {
        if (unlock_block_modification || left_mouse_last == GLFW_RELEASE || 
            last_break + std::chrono::milliseconds(300) < std::chrono::steady_clock::now())
        {
            changed_block_pos = m_camera->m_ray_cast.block_pos;
            m_world->set_block_at(changed_block_pos.value(), 0);
            m_world->remove_light(changed_block_pos.value());
            last_break = std::chrono::steady_clock::now();
        }
        left_mouse_last = GLFW_PRESS;
    } else {
        left_mouse_last = GLFW_RELEASE;
    }

    static bool right_mouse_last = GLFW_RELEASE;
    static auto last_place = std::chrono::steady_clock::now();
    if (m_window->is_mouse_button_pressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        if (unlock_block_modification || right_mouse_last == GLFW_RELEASE || 
            last_place + std::chrono::milliseconds(300) < std::chrono::steady_clock::now())
        {
            i32vec3 block_pos = m_camera->m_ray_cast.block_pos + m_camera->m_ray_cast.block_normal;
            if (!aabb_intersect(m_camera->m_pos + vec3(-0.3, -1.5, -0.3), m_camera->m_pos + vec3(0.3, 0.3, 0.3), vec3(block_pos), vec3(block_pos) + 1.0f)) {
                changed_block_pos = block_pos;
                m_world->set_block_at(changed_block_pos.value(), block_to_place);
                if (block_to_place == 1)
                    m_world->add_light(changed_block_pos.value(), (u16)0xF00F);
                else if (block_to_place == 2)
                    m_world->add_light(changed_block_pos.value(), (u16)0xF0F0);
                else if (block_to_place == 3)
                    m_world->add_light(changed_block_pos.value(), (u16)0xFF20);
                else if (block_to_place == 4)
                    m_world->add_light(changed_block_pos.value(), (u16)0xF2FF);
                else
                    m_world->add_light(changed_block_pos.value(), (u16)0xFFFF);
                last_place = std::chrono::steady_clock::now();
            }
        }
        right_mouse_last = GLFW_PRESS;
    } else {
        right_mouse_last = GLFW_RELEASE;
    }

    if (m_camera->is_free()) {
        if (m_window->is_key_pressed(GLFW_KEY_W))
            m_camera->process_free_movement(MovementDirection::FORWARD, m_render_delta_time);
        if (m_window->is_key_pressed(GLFW_KEY_A))
            m_camera->process_free_movement(MovementDirection::LEFT, m_render_delta_time);
        if (m_window->is_key_pressed(GLFW_KEY_S))
            m_camera->process_free_movement(MovementDirection::BACKWARD, m_render_delta_time);
        if (m_window->is_key_pressed(GLFW_KEY_D))
            m_camera->process_free_movement(MovementDirection::RIGHT, m_render_delta_time);

        if (!override_camera_speed) {
            if (m_window->is_key_pressed(GLFW_KEY_LEFT_CONTROL))
                m_camera->m_free_speed = m_camera->fast_free_speed;
            else if (m_window->is_key_pressed(GLFW_KEY_LEFT_SHIFT))
                m_camera->m_free_speed = m_camera->slow_free_speed;
            else
                m_camera->m_free_speed = m_camera->normal_free_speed;
        }
    } else {
        vec3 horizontal_front = m_camera->m_front;
        horizontal_front.y = 0;
        horizontal_front = glm::normalize(horizontal_front);
        vec3 applied_velocity(0, 0, 0);
        if (m_window->is_key_pressed(GLFW_KEY_W))
            applied_velocity += horizontal_front;
        if (m_window->is_key_pressed(GLFW_KEY_S))
            applied_velocity -= horizontal_front;
        if (m_window->is_key_pressed(GLFW_KEY_A))
            applied_velocity += m_camera->m_right;
        if (m_window->is_key_pressed(GLFW_KEY_D))
            applied_velocity -= m_camera->m_right;
        if (applied_velocity.x != 0 || applied_velocity.y != 0 || applied_velocity.z != 0) {
            applied_velocity = glm::normalize(applied_velocity);
            if (m_window->is_key_pressed(GLFW_KEY_LEFT_CONTROL)) {
                applied_velocity *= 7;
                target_fov = m_camera->default_fov + 10;
            } else if (m_window->is_key_pressed(GLFW_KEY_LEFT_SHIFT)) {
                applied_velocity *= 2;
            } else {
                applied_velocity *= 4;
                target_fov = m_camera->default_fov;
            }
        }
        if (m_window->is_key_pressed(GLFW_KEY_SPACE)) {
            auto pos = m_registry.get<Movement>(m_local_player).pos;
            if (get_block_data(m_world->get_block_at(glm::floor(pos + vec3(-0.3f, -1.501f, -0.3f))))->m_collidable ||
                get_block_data(m_world->get_block_at(glm::floor(pos + vec3(-0.3f, -1.501f,  0.3f))))->m_collidable ||
                get_block_data(m_world->get_block_at(glm::floor(pos + vec3( 0.3f, -1.501f, -0.3f))))->m_collidable ||
                get_block_data(m_world->get_block_at(glm::floor(pos + vec3( 0.3f, -1.501f,  0.3f))))->m_collidable)
            {
                applied_velocity.y = 7.0f;
            }
        }

        std::scoped_lock guard(movement_velocity_mutex);
        movement_velocity = applied_velocity;
    }
}
