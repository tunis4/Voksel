#pragma once

#include "gl/shader.hpp"
#include "gl/mesh.hpp"
#include "gl/framebuffer.hpp"
#include "window.hpp"
#include "camera.hpp"
#include "selection_box.hpp"
#include "world/sky.hpp"

struct ScreenQuadVertex {
    glm::vec2 pos;
    glm::vec2 tex;

    static void setup_attrib_pointers() {
        glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(ScreenQuadVertex), (void*)offsetof(ScreenQuadVertex, pos));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(ScreenQuadVertex), (void*)offsetof(ScreenQuadVertex, tex));
        glEnableVertexAttribArray(1);
    }
};

class Renderer {
    Window *m_window;
    Framebuffer *m_framebuffer;
    Mesh<ScreenQuadVertex> *m_screen_quad;
    Shader *m_screen_shader;
    Camera *m_camera;

    Sky *m_sky;
    SelectionBox *m_selection_box;
    
public:
    Renderer(Window *window, Camera *camera);
    ~Renderer();

    void render();

    void on_framebuffer_resize(uint width, uint height);
    void on_cursor_move(f64 x, f64 y);
};
