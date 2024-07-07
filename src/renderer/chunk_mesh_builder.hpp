#pragma once

#include <vector>

#include "../util.hpp"

class Chunk;

namespace renderer {
    struct BlockFace;
    struct ChunkTransparentVertex;
    struct ChunkMesh;
    class ChunkRenderer;

    class ChunkMeshBuilder {
        void push_transparent_face_indices();

    public:
        Chunk *m_chunk;
        std::vector<BlockFace> m_faces;
        std::vector<ChunkTransparentVertex> m_transparent_vertices;
        std::vector<u32> m_transparent_indices;
        i32 m_last_transparent_index;

        bool build(Chunk *chunk);

        ChunkMeshBuilder();
        ~ChunkMeshBuilder();
    };
}
