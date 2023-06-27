#pragma once

#include <string>
#include <functional>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../util/util.hpp"

namespace client {
    // Singleton
    class Window {
        std::function<void(uint width, uint height)> m_framebuffer_size_callback;
        std::function<void(f64 x, f64 y)> m_cursor_pos_callback;

    public:
        GLFWwindow *m_glfw_window;
        uint m_width, m_height;

        bool m_cursor_enabled;
        uint m_cursor_x, m_cursor_y;

        Window(int window_width, int window_height, std::string window_title);
        ~Window();

        static Window* get();

        void close();

        void enable_cursor();
        void disable_cursor();
        void toggle_cursor();

        bool is_key_pressed(int glfw_key);
        bool is_mouse_button_pressed(int glfw_mouse_button);

        inline bool should_close() const { return glfwWindowShouldClose(m_glfw_window); }

        void set_framebuffer_size_callback(std::function<void(uint width, uint height)> callback);
        void set_cursor_pos_callback(std::function<void(f64 x, f64 y)> callback);
    };
}
