#pragma once

#include <string>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../util.hpp"

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

    template<typename F>
    void loop(F f) {
        while (!glfwWindowShouldClose(m_window)) {
            f();
            glfwSwapBuffers(m_window);
            glfwPollEvents();
        }
    }

    void close();
    void vsync(bool enabled);

    bool is_cursor_enabled() const;
    void enable_cursor();
    void disable_cursor();
    void toggle_cursor();

    bool is_key_pressed(int glfw_key);
    bool is_mouse_button_pressed(int glfw_mouse_button);

    GLFWwindow* get_glfw_window();
    GLADloadproc get_loadproc();

    void set_framebuffer_size_callback(std::function<void(uint width, uint height)> callback);
    void set_cursor_pos_callback(std::function<void(f64 x, f64 y)> callback);
};
