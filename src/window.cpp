#include "window.hpp"

#include <stdexcept>

static Window *window_singleton = nullptr;

Window::Window(int window_width, int window_height, std::string window_title) {
    if (window_singleton)
        throw std::runtime_error("The Window object has already been created");

    window_singleton = this;

    m_width = window_width;
    m_height = window_height;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    m_glfw_window = glfwCreateWindow(window_width, window_height, window_title.c_str(), NULL, NULL);
    if (!m_glfw_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
}

Window::~Window() {
    glfwDestroyWindow(m_glfw_window);
    glfwTerminate();
    window_singleton = nullptr;
}

Window* Window::get() {
    return window_singleton;
}

void Window::set_framebuffer_size_callback(std::function<void(uint width, uint height)> callback) {
    m_framebuffer_size_callback = callback;
    glfwSetFramebufferSizeCallback(m_glfw_window, [] (GLFWwindow *, int width, int height) {
        window_singleton->m_width = width;
        window_singleton->m_height = height;
        window_singleton->m_framebuffer_size_callback(static_cast<uint>(width), static_cast<uint>(height));
    });
}

void Window::set_cursor_pos_callback(std::function<void(f64 x, f64 y)> callback) {
    m_cursor_pos_callback = callback;
    glfwSetCursorPosCallback(m_glfw_window, [] (GLFWwindow *, f64 x, f64 y) {
        window_singleton->m_cursor_pos_callback(x, y);
    });
}

void Window::close() {
    glfwSetWindowShouldClose(m_glfw_window, true);
}

void Window::enable_cursor() {
    glfwSetInputMode(m_glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    m_cursor_enabled = true;
    m_cursor_x = m_width / 2;
    m_cursor_y = m_height / 2;
}

void Window::disable_cursor() {
    glfwSetInputMode(m_glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    m_cursor_enabled = false;
    m_cursor_x = m_width / 2;
    m_cursor_y = m_height / 2;
}

void Window::toggle_cursor() {
    if (m_cursor_enabled) disable_cursor();
    else enable_cursor();
}

bool Window::is_key_pressed(int glfw_key) {
    return glfwGetKey(m_glfw_window, glfw_key) == GLFW_PRESS;
}

bool Window::is_mouse_button_pressed(int glfw_mouse_button) {
    return glfwGetMouseButton(m_glfw_window, glfw_mouse_button) == GLFW_PRESS;
}
