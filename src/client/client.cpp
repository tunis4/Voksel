#include "client.hpp"
#include "renderer/chunk_renderer.hpp"
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
            if (!m_window->is_cursor_enabled()) {
                m_camera->process_mouse_movement(x, y);
                m_renderer.on_cursor_move(x, y);
            }
        });

        m_camera = new Camera(glm::vec3(16 * 2, 16 * 2, 16 * 2));
        m_camera->set_free(true);

        m_renderer.init(m_window, m_camera);

        block::register_blocks();
        m_world = new world::World();
        
        for (usize i = 0; i < block::num_block_data(); i++)
            m_world->set_block_at(glm::i32vec3(24 + i * 2, 24, 24), i);
        
        entt::locator<render::ChunkRenderer>::emplace(m_renderer.m_context).init();
    }

    Client::~Client() {
        m_renderer.begin_cleanup();
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

    void Client::loop() {
        while (!m_window->should_close()) {
            auto frame_start = std::chrono::high_resolution_clock::now();

            ImGui_ImplVoksel_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            static bool show_demo_window = false;
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);
            
            ImGui::Begin("voksel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            if (ImGui::Button("Show ImGui demo window"))
                show_demo_window = !show_demo_window;

            ImGui::Text("Camera position: %d, %d, %d", m_camera->block_pos().x, m_camera->block_pos().y, m_camera->block_pos().z);
            ImGui::Checkbox("Override camera speed", &override_camera_speed);
            if (override_camera_speed)
                ImGui::SliderFloat("Camera speed", &m_camera->m_free_speed, 5, 100);

            process_input();
            m_camera->update_matrices(m_window->width(), m_window->height());
            m_camera->m_frustum.update(m_camera->m_projection_matrix * m_camera->m_view_matrix);
            
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
        
        static bool tab_locked = false;
        if (m_window->is_key_pressed(GLFW_KEY_TAB)) {
            if (!tab_locked) {
                m_window->toggle_cursor();
                tab_locked = true;
            }
        } else tab_locked = false;

        static bool f_locked = false;
        if (m_window->is_key_pressed(GLFW_KEY_F)) {
            if (!f_locked) {
                m_camera->set_free(!m_camera->is_free());
                f_locked = true;
            }
        } else f_locked = false;

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
