#pragma once

#include <vector>

#include "chunk_renderer.hpp"

namespace render {
    class ChunkMeshBuilder {
        ChunkRenderer *m_renderer;
        
        void push_face_indices(bool flipped);
    
    public:
        std::vector<ChunkVertex> m_vertices;
        std::vector<u32> m_indices;
        i32 m_last_index;
        ChunkRender *m_current;
        bool m_finished = false;

        i64 build();

        ChunkMeshBuilder(ChunkRenderer *renderer);
        ~ChunkMeshBuilder();
    };
}
