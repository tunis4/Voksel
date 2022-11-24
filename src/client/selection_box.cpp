#include "selection_box.hpp"

SelectionBox::SelectionBox() {
    LineVertex selection_box_vertices[] = {
        {{ 0, 0, 0 }},
        {{ 0, 0, 1 }},
        {{ 0, 1, 0 }},
        {{ 1, 0, 0 }},
        {{ 1, 0, 1 }},
        {{ 0, 1, 1 }},
        {{ 1, 1, 0 }},
        {{ 1, 1, 1 }}
    };
    
    u32 selection_box_indices[] = {
        0, 1, 0, 2, 0, 3, 
        1, 4, 3, 4, 2, 5,
        2, 6, 5, 1, 5, 7,
        6, 3, 6, 7, 7, 4
    };

    m_mesh = new Mesh<LineVertex, GL_LINES>(selection_box_vertices, selection_box_indices, 8, 24);
    m_shader = new Shader("res/shaders/selection_box.vs.glsl", "res/shaders/selection_box.fs.glsl");
}

SelectionBox::~SelectionBox() {
    delete m_mesh;
    delete m_shader;
}

void SelectionBox::render(Camera *camera) {
    m_shader->use();
    m_shader->set_vec4("color", glm::vec4(0, 0, 0, 1));
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0, 0, 0));
    m_shader->set_mat4("model", model);
    m_shader->set_mat4("view", camera->m_view_matrix);
    m_shader->set_mat4("projection", camera->m_projection_matrix);
    m_mesh->render();
}
