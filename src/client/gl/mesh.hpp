#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "../../util.hpp"

template<class F>
concept VertexFormat = requires {
    F::setup_attrib_pointers();
};

class VAO {
    uint m_id;

public:
    VAO() {
        glGenVertexArrays(1, &m_id);
    }

    ~VAO() {
        glDeleteVertexArrays(1, &m_id);
    }

    void bind() {
        glBindVertexArray(m_id);
    }

    template<VertexFormat F>
    void setup_attrib_pointers() {
        bind();
        F::setup_attrib_pointers();
    }
};

template<typename T, GLenum target>
class BufferObject {
    uint m_id;

public:
    BufferObject() {
        glGenBuffers(1, &m_id);
    }

    ~BufferObject() {
        glDeleteBuffers(1, &m_id);
    }

    void bind() {
        glBindBuffer(target, m_id);
    }

    void upload_data(T *data, usize size) {
        bind();
        glBufferData(target, size, data, GL_STATIC_DRAW);
    }
};

template<VertexFormat F> using VBO = BufferObject<F, GL_ARRAY_BUFFER>;
using EBO = BufferObject<u32, GL_ELEMENT_ARRAY_BUFFER>;

template<VertexFormat F, int primitive = GL_TRIANGLES>
class Mesh {
    VAO m_vao;
    VBO<F> m_vbo;
    EBO m_ebo;
    u32 m_index_count;

public:
    Mesh(F *vertices, u32 *indices, u32 vertex_count, u32 index_count) : m_index_count(index_count) {
        m_vao.bind();
        m_vbo.upload_data(vertices, vertex_count * sizeof(F));
        m_ebo.upload_data(indices, index_count * sizeof(u32));
        m_vao.setup_attrib_pointers<F>();
    }

    ~Mesh() = default;

    void render() {
        m_vao.bind();
        glDrawElements(primitive, m_index_count, GL_UNSIGNED_INT, 0);
    }
};
