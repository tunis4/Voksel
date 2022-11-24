#pragma once

#include "gl/mesh.hpp"
#include "gl/shader.hpp"
#include "camera.hpp"

struct LineVertex {
    glm::vec3 pos;

    static void setup_attrib_pointers() {
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(LineVertex), (void*)offsetof(LineVertex, pos));
        glEnableVertexAttribArray(0);
    }
};

class SelectionBox {
    Mesh<LineVertex, GL_LINES> *m_mesh;
    Shader *m_shader;

public:
    SelectionBox();
    ~SelectionBox();

    void render(Camera *camera);
};
