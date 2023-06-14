#pragma once

#include <memory>
#include <glm/ext/vector_int3_sized.hpp>

#include "../block/block.hpp"

class Chunk {
public:
    static constexpr i32 size = 32;
    static constexpr i32 volume = size * size * size;

    block::NID m_blocks[volume];
    u32 m_light_map[volume];

    Chunk();
    ~Chunk();

    block::NID get_block_at(glm::i32vec3 offset);
    void set_block_at(glm::i32vec3 offset, block::NID block_nid);
};
