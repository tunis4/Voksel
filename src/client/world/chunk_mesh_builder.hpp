#pragma once

#include <thread>

class ChunkMeshBuilder {
    std::thread m_thread;

public:
    ChunkMeshBuilder();
    ~ChunkMeshBuilder();
};
