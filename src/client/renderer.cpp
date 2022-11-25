#include "renderer.hpp"

Renderer::Renderer(Window *window) : m_window(window) {
    ScreenQuadVertex screen_quad_vertices[] = {
        // positions         tex coords
        {{ -1.0f, -1.0f }, { 0.0f, 0.0f }},
        {{ -1.0f, +1.0f }, { 0.0f, 1.0f }},
        {{ +1.0f, -1.0f }, { 1.0f, 0.0f }},
        {{ +1.0f, +1.0f }, { 1.0f, 1.0f }}
    };

    u32 screen_quad_indices[] = {
        1, 0, 2, 1, 2, 3
    };

    m_screen_quad = new Mesh(screen_quad_vertices, screen_quad_indices, 4, 6);
    m_screen_shader = new Shader("res/shaders/screen.vs.glsl", "res/shaders/screen.fs.glsl");
    m_framebuffer = new Framebuffer(window->width(), window->height());

    m_camera = new Camera(glm::vec3(0, 0, -2));

    m_selection_box = new SelectionBox();
}

Renderer::~Renderer() {
    
}

void Renderer::render() {
    m_framebuffer->bind();
    glEnable(GL_DEPTH_TEST);

    glClearColor(powf(0.53, 2.2), powf(0.81, 2.2), powf(0.98, 2.2), 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_camera->update_matrices(m_window->width(), m_window->height());
    
    m_selection_box->render(m_camera);

    Framebuffer::unbind();
    glDisable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    m_screen_shader->use();
    m_screen_shader->set_int("screen_texture", 0);
    m_framebuffer->bind_color_buffer();
    m_screen_quad->render();
}

void Renderer::on_framebuffer_resize(uint width, uint height) {
    m_framebuffer->resize(width, height);
}

void Renderer::on_cursor_move(f64 x, f64 y) {
    m_camera->process_mouse_movement(x, y);
}