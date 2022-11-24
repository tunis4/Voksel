#pragma once

#include "../gl/mesh.hpp"
#include "../gl/shader.hpp"

class Sky {
    struct PlaneVertex {
        glm::vec2 pos;

        static void setup_attrib_pointers() {
            glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(PlaneVertex), (void*)offsetof(PlaneVertex, pos));
            glEnableVertexAttribArray(0);
        }
    };

    Mesh<PlaneVertex> *m_plane_mesh;
    Shader *m_plane_shader;

public:
    Sky();
    ~Sky();
    
    void render();
};
