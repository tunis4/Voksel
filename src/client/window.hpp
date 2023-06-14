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
        GLFWwindow *m_window;
        uint m_width, m_height;
        bool m_cursor_enabled;

        std::function<void(uint width, uint height)> m_framebuffer_size_callback;
        std::function<void(f64 x, f64 y)> m_cursor_pos_callback;

    public:
        Window(int window_width, int window_height, std::string window_title);
        ~Window();

        static Window* get();

        void close();

        inline uint width() const { return m_width; }
        inline uint height() const { return m_height; }

        inline bool is_cursor_enabled() const { return m_cursor_enabled; }
        void enable_cursor();
        void disable_cursor();
        void toggle_cursor();

        bool is_key_pressed(int glfw_key);
        bool is_mouse_button_pressed(int glfw_mouse_button);

        inline GLFWwindow* glfw_window() const { return m_window; }
        inline bool should_close() const { return glfwWindowShouldClose(m_window); }

        void set_framebuffer_size_callback(std::function<void(uint width, uint height)> callback);
        void set_cursor_pos_callback(std::function<void(f64 x, f64 y)> callback);
    };
}
