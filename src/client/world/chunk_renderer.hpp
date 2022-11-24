#pragma once

#include <thread>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "../gl/mesh.hpp"
#include "../../world/world.hpp"

struct ChunkVertex {
    glm::vec3 pos;
    u32 tex; // first 30 bits for layer, last 2 bits for index
    glm::vec4 light;
    f32 sunlight;

    inline ChunkVertex() {};
    inline ChunkVertex(glm::vec3 pos, u32 tex_index, u32 tex_layer, u32 a, u32 b, u32 c, u32 d) : pos(pos) {
        tex = ((tex_index & 0b11) << 30) | (tex_layer & ~((u32)0b11 << 30));
        light.r = (f32)((a & 0xF) + (b & 0xF) + (c & 0xF) + (d & 0xF)) / 60.0f;
        light.g = (f32)((a >> 4 & 0xF) + (b >> 4 & 0xF) + (c >> 4 & 0xF) + (d >> 4 & 0xF)) / 60.0f;
        light.b = (f32)((a >> 8 & 0xF) + (b >> 8 & 0xF) + (c >> 8 & 0xF) + (d >> 8 & 0xF)) / 60.0f;
        light.a = (f32)((a >> 12 & 0xF) + (b >> 12 & 0xF) + (c >> 12 & 0xF) + (d >> 12 & 0xF)) / 60.0f;
        sunlight = (f32)((a >> 16 & 0xF) + (b >> 16 & 0xF) + (c >> 16 & 0xF) + (d >> 16 & 0xF)) / 60.0f;
    }

    static void setup_attrib_pointers() {
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, pos));
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, tex));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, light));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 1, GL_FLOAT, false, sizeof(ChunkVertex), (void*)offsetof(ChunkVertex, sunlight));
        glEnableVertexAttribArray(3);
    }
};

class ChunkRenderable {
public:
    glm::i32vec3 m_chunk_pos;
    Mesh<ChunkVertex> m_mesh;
};

class ChunkMeshBuilder {
    std::thread m_thread;
    std::vector<ChunkVertex> m_vertices;
    std::vector<u32> m_indices;
    std::shared_ptr<ChunkRenderable> current;

public:
    ChunkMeshBuilder();
    ~ChunkMeshBuilder();
};

class ChunkRenderer {
    
};
