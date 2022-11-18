#pragma once

#include "chunk.hpp"

class Region {
public:
    static constexpr i32 size = 16; 
    static constexpr i32 volume = size * size * size;

    Region();
    ~Region();

    std::shared_ptr<Chunk>& get_chunk(glm::i32vec3 offset);

private:
    std::shared_ptr<Chunk> m_chunks[volume];
};
