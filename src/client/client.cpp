#include "client.hpp"

namespace client {
    static Client *client_singleton = nullptr;

    Client::Client() {
        if (client_singleton)
            throw std::runtime_error("The Client object has already been created");

        client_singleton = this;
        
        m_window = new Window(1280, 720, "Voksel");
        m_window->disable_cursor();

        m_window->set_framebuffer_size_callback([&] (uint width, uint height) {
            m_renderer->on_framebuffer_resize(width, height);
        });

        m_window->set_cursor_pos_callback([&] (f64 x, f64 y) {
            if (!m_window->is_cursor_enabled()) {
                m_camera->process_mouse_movement(x, y);
                m_renderer->on_cursor_move(x, y);
            }
        });

        m_camera = new Camera(glm::vec3(16, 16, 16));
        m_camera->set_free(true);

        block::register_blocks();
        m_world = new World();
        m_renderer = new render::Renderer(m_window, m_camera);
    }

    Client::~Client() {
        client_singleton = nullptr;
    }
    
    Client* Client::get() {
        return client_singleton;
    }

    void Client::loop() {
        while (!m_window->should_close()) {
            f64 current_frame = glfwGetTime();
            m_delta_time = current_frame - m_last_frame;
            m_last_frame = current_frame;

            process_input();
            m_camera->update_matrices(m_window->width(), m_window->height());
            m_renderer->render();

            // m_chunk_manager->build_meshes();

            glfwPollEvents();
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
            
            if (m_window->is_key_pressed(GLFW_KEY_LEFT_CONTROL))
                m_camera->m_free_speed = m_camera->fast_free_speed;
            else if (m_window->is_key_pressed(GLFW_KEY_LEFT_SHIFT))
                m_camera->m_free_speed = m_camera->slow_free_speed;
            else
                m_camera->m_free_speed = m_camera->normal_free_speed;
        }
    }
}
