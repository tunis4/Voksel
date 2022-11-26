#include "sky.hpp"

Sky::Sky() {
    PlaneVertex plane_vertices[] = {
        {{ -0.5f, -0.5f }},
        {{ -0.5f, +0.5f }},
        {{ +0.5f, -0.5f }},
        {{ +0.5f, +0.5f }}
    };

    u32 plane_indices[] = {
        1, 0, 2, 1, 2, 3
    };
    
    m_plane_mesh = new Mesh(plane_vertices, plane_indices, 4, 6);
    m_plane_shader = new Shader("res/shaders/sky.vs.glsl", "res/shaders/sky.fs.glsl");
}

Sky::~Sky() {
    
}

void Sky::render(Camera *camera) {
    glDepthMask(false);

    m_plane_shader->use();
    m_plane_shader->set_vec4("fog_color", glm::vec4(0 / 255.0f, 197 / 255.0f, 255 / 255.0f, 1));
    m_plane_shader->set_float("fog_near", 32);
    m_plane_shader->set_float("fog_far", 256);
    m_plane_shader->set_mat4("view", camera->m_view_matrix);
    m_plane_shader->set_mat4("projection", camera->m_projection_matrix);

    // sky plane
    m_plane_shader->set_vec4("color", glm::vec4(255 / 255.0f, 255 / 255.0f, 255 / 255.0f, 1));
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, camera->pos() + glm::vec3(0, 16, 0));
    model = glm::scale(model, glm::vec3(1024, 1, 1024));
    m_plane_shader->set_mat4("model", model);
    m_plane_mesh->render();

    // void plane
    m_plane_shader->set_vec4("color", glm::vec4(0 / 255.0f, 0 / 255.0f, 0 / 255.0f, 1));
    model = glm::mat4(1.0f);
    model = glm::translate(model, camera->pos() + glm::vec3(0, -16, 0));
    model = glm::scale(model, glm::vec3(1024, 1, 1024));
    m_plane_shader->set_mat4("model", model);
    m_plane_mesh->render();

    glDepthMask(true);
}
