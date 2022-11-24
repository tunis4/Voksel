#include "chunk_renderer.hpp"

ChunkMeshBuilder::ChunkMeshBuilder() {
    m_thread = std::thread([this] {
        
    });
}

ChunkMeshBuilder::~ChunkMeshBuilder() {
    if (m_thread.joinable())
        m_thread.join();
}
