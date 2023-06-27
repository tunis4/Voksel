#include "client.hpp"
#include "renderer/chunk_renderer.hpp"
#include "renderer/box_renderer.hpp"
#include "renderer/imgui_impl_voksel.hpp"

#include <chrono>
#include <entt/entt.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>

namespace client {
    static Client *client_singleton = nullptr;

    Client::Client() {
        if (client_singleton)
            throw std::runtime_error("The Client object has already been created");

        client_singleton = this;
        
        m_window = new Window(1280, 720, "Voksel");
        m_window->enable_cursor();

        m_window->set_framebuffer_size_callback([&] (uint width, uint height) {
            m_renderer.on_framebuffer_resize(width, height);
        });

        m_window->set_cursor_pos_callback([&] (f64 x, f64 y) {
            if (!m_window->m_cursor_enabled) {
                m_camera->process_mouse_movement(x, y);
                m_renderer.on_cursor_move(x, y);
            } else {
                m_window->m_cursor_x = x;
                m_window->m_cursor_y = y;
            }
        });

        m_camera = new Camera(glm::vec3(16, 16, 16));
        m_camera->set_free(true);

        m_renderer.init(m_window, m_camera);

        block::register_blocks();
        m_world = new world::World();
        
        for (usize i = 0; i < block::num_block_data(); i++)
            m_world->set_block_at(glm::i32vec3(24 + i * 2, 24, -1 + (i % 2) * 2), i);
        
        entt::locator<render::ChunkRenderer>::emplace(m_renderer.m_context).init();
        entt::locator<render::BoxRenderer>::emplace(m_renderer.m_context).init();
    }

    Client::~Client() {
        m_renderer.begin_cleanup();
        entt::locator<render::BoxRenderer>::value().cleanup();
        entt::locator<render::ChunkRenderer>::value().cleanup();
        m_renderer.cleanup();
        delete m_world;
        delete m_camera;
        delete m_window;
        client_singleton = nullptr;
    }
    
    Client* Client::get() {
        return client_singleton;
    }
    
    static bool override_camera_speed = false;
    static bool unlock_block_modification = false;

    void Client::loop() {
        while (!m_window->should_close()) {
            auto frame_start = std::chrono::high_resolution_clock::now();

            ImGui_ImplVoksel_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            static bool show_demo_window = false;
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);
            
            if (!m_window->m_cursor_enabled) {
                ImGui::Begin("Crosshair", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
                auto draw = ImGui::GetBackgroundDrawList();
                draw->AddCircleFilled(ImVec2(m_window->m_width / 2.0f, m_window->m_height / 2.0f), 2.0f, IM_COL32(255, 255, 255, 255));
                ImGui::End();
            }

            ImGui::Begin("voksel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            if (ImGui::Button("Show ImGui demo window"))
                show_demo_window = !show_demo_window;

            ImGui::Text("Press G to toggle cursor grab");
            ImGui::Checkbox("Unlock block modification", &unlock_block_modification);
            ImGui::Text("Camera position: %d, %d, %d", (i32)m_camera->block_pos().x, (i32)m_camera->block_pos().y, (i32)m_camera->block_pos().z);
            ImGui::Checkbox("Override camera speed", &override_camera_speed);
            if (override_camera_speed)
                ImGui::SliderFloat("Camera speed", &m_camera->m_free_speed, 5, 100);

            process_input();
            m_camera->update_matrices(m_window->m_width, m_window->m_height);
            m_camera->m_frustum.update(m_camera->m_projection_matrix * m_camera->m_view_matrix);

            m_camera->m_ray_cast = m_world->cast_ray(m_camera->pos(), m_camera->pos() + m_camera->m_front * 10.0f);
            if (m_camera->m_ray_cast.hit) {
                entt::locator<render::BoxRenderer>::value().set_box(true, glm::vec3(m_camera->m_ray_cast.block_pos));
            } else {
                entt::locator<render::BoxRenderer>::value().set_box(false);
            }
            
            entt::locator<render::ChunkRenderer>::value().update(m_delta_time);
            m_renderer.render(m_delta_time);

            glfwPollEvents();

            auto frame_end = std::chrono::high_resolution_clock::now();
            m_delta_time = std::chrono::duration_cast<std::chrono::duration<f64>>(frame_end - frame_start).count(); // cast to seconds as f64
        }
    }

    void Client::process_input() {
        if (m_window->is_key_pressed(GLFW_KEY_ESCAPE))
            m_window->close();
        
        static bool g_locked = false;
        if (m_window->is_key_pressed(GLFW_KEY_G)) {
            if (!g_locked) {
                m_window->toggle_cursor();
                g_locked = true;
            }
        } else g_locked = false;

        static bool f_locked = false;
        if (m_window->is_key_pressed(GLFW_KEY_F)) {
            if (!f_locked) {
                m_camera->set_free(!m_camera->is_free());
                f_locked = true;
            }
        } else f_locked = false;

        // glm::vec4 ray_clip;
        // ray_clip.x = 1.0f - ((2.0f * m_context.window->m_cursor_x) / m_context.window->m_width);
        // ray_clip.y = 1.0f - ((2.0f * m_context.window->m_cursor_y) / m_context.window->m_height);
        // ray_clip.z = -1.0f;
        // ray_clip.w = 1.0f;
        // glm::vec4 ray_eye = glm::inverse(m_context.camera->m_projection_matrix) * ray_clip;
        // ray_eye.z = -1.0f;
        // ray_eye.w = 0.0f;
        // glm::mat4 view = m_context.camera->m_view_matrix;
        // view[1][1] *= -1.0f;
        // glm::vec3 ray_world = glm::vec3(glm::inverse(view) * ray_eye);
        // ray_world.x *= -1.0f;
        // ray_world.z *= -1.0f;
        // ray_world = glm::normalize(ray_world);
        // util::log(util::INFO, "ray", "x: {}, y: {}, z: {}", ray_world.x, ray_world.y, ray_world.z);    
        // auto ray_result = world->cast_ray(m_context.camera->pos(), m_context.camera->pos() + ray_world * 30.0f);

        static block::NID block_to_place = 5;
        if (m_window->is_key_pressed(GLFW_KEY_1)) block_to_place = 1;
        if (m_window->is_key_pressed(GLFW_KEY_2)) block_to_place = 2;
        if (m_window->is_key_pressed(GLFW_KEY_3)) block_to_place = 3;
        if (m_window->is_key_pressed(GLFW_KEY_4)) block_to_place = 4;
        if (m_window->is_key_pressed(GLFW_KEY_5)) block_to_place = 5;
        if (m_window->is_key_pressed(GLFW_KEY_6)) block_to_place = 6;
        if (m_window->is_key_pressed(GLFW_KEY_7)) block_to_place = 7;
        if (m_window->is_key_pressed(GLFW_KEY_8)) block_to_place = 8;
        if (m_window->is_key_pressed(GLFW_KEY_9)) block_to_place = 9;
        
        std::optional<glm::i32vec3> changed_block_pos;
        static bool left_mouse_last = GLFW_RELEASE;
        if (m_window->is_mouse_button_pressed(GLFW_MOUSE_BUTTON_LEFT)) {
            if (unlock_block_modification || left_mouse_last == GLFW_RELEASE) {
                changed_block_pos = m_camera->m_ray_cast.block_pos;
                m_world->set_block_at(changed_block_pos.value(), 0);
            }
            left_mouse_last = GLFW_PRESS;
        } else {
            left_mouse_last = GLFW_RELEASE;
        }
        
        static bool right_mouse_last = GLFW_RELEASE;
        if (m_window->is_mouse_button_pressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            if (unlock_block_modification || right_mouse_last == GLFW_RELEASE) {
                changed_block_pos = m_camera->m_ray_cast.block_pos + m_camera->m_ray_cast.block_normal;
                m_world->set_block_at(changed_block_pos.value(), block_to_place);
            }
            right_mouse_last = GLFW_PRESS;
        } else {
            right_mouse_last = GLFW_RELEASE;
        }

        if (changed_block_pos.has_value()) {
            auto &chunk_renderer = entt::locator<render::ChunkRenderer>::value();
            auto [hit_chunk_pos, hit_block_offset] = util::signed_i32vec3_divide(changed_block_pos.value(), world::Chunk::size);
            chunk_renderer.remesh_chunk_urgent(hit_chunk_pos);
            if (hit_block_offset.x == 0) chunk_renderer.remesh_chunk_urgent(hit_chunk_pos + glm::i32vec3(-1, 0, 0));
            else if (hit_block_offset.x == world::Chunk::size - 1) chunk_renderer.remesh_chunk_urgent(hit_chunk_pos + glm::i32vec3(1, 0, 0));
            if (hit_block_offset.y == 0) chunk_renderer.remesh_chunk_urgent(hit_chunk_pos + glm::i32vec3(0, -1, 0));
            else if (hit_block_offset.y == world::Chunk::size - 1) chunk_renderer.remesh_chunk_urgent(hit_chunk_pos + glm::i32vec3(0, 1, 0));
            if (hit_block_offset.z == 0) chunk_renderer.remesh_chunk_urgent(hit_chunk_pos + glm::i32vec3(0, 0, -1));
            else if (hit_block_offset.z == world::Chunk::size - 1) chunk_renderer.remesh_chunk_urgent(hit_chunk_pos + glm::i32vec3(0, 0, 1));
        }

        if (m_camera->is_free()) {
            if (m_window->is_key_pressed(GLFW_KEY_W))
                m_camera->process_free_movement(util::MovementDirection::FORWARD, m_delta_time);
            if (m_window->is_key_pressed(GLFW_KEY_A))
                m_camera->process_free_movement(util::MovementDirection::LEFT, m_delta_time);
            if (m_window->is_key_pressed(GLFW_KEY_S))
                m_camera->process_free_movement(util::MovementDirection::BACKWARD, m_delta_time);
            if (m_window->is_key_pressed(GLFW_KEY_D))
                m_camera->process_free_movement(util::MovementDirection::RIGHT, m_delta_time);
            
            if (!override_camera_speed) {
                if (m_window->is_key_pressed(GLFW_KEY_LEFT_CONTROL))
                    m_camera->m_free_speed = m_camera->fast_free_speed;
                else if (m_window->is_key_pressed(GLFW_KEY_LEFT_SHIFT))
                    m_camera->m_free_speed = m_camera->slow_free_speed;
                else
                    m_camera->m_free_speed = m_camera->normal_free_speed;
            }
        }
    }
}
