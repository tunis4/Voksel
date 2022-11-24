#include "sky.hpp"

Sky::Sky() {
    PlaneVertex plane_vertices[] = {
        {{ -0.5f, -0.5f }},
        {{ -0.5f, +0.5f }},
        {{ +0.5f, -0.5f }},
        {{ +0.5f, +0.5f }}
    };

    u32 plane_indices[] = {
        3, 0, 1, 3, 1, 2
    };
    
    m_plane_mesh = new Mesh(plane_vertices, plane_indices, 4, 6);
    m_plane_shader = new Shader("res/shaders/sky.vs.glsl", "res/shaders/sky.fs.glsl");
}

Sky::~Sky() {
    
}

void Sky::render() {
    glDepthMask(false);
    m_plane_shader->use();
    m_plane_mesh->render();
    glDepthMask(true);
}
