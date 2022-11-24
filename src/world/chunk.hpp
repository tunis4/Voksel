#pragma once

#include <memory>
#include <glm/ext/vector_int3_sized.hpp>

#include "../block/block.hpp"

class Chunk {
public:
    static constexpr i32 size = 16; 
    static constexpr i32 volume = size * size * size; 

private:
    BlockNID m_blocks[volume];
    u32 m_light_map[volume];

    inline BlockNID* __get_block_at(i32 x, i32 y, i32 z) {
        return &m_blocks[x * size * size + y * size + z];
    }

public:
    Chunk();
    ~Chunk();

    BlockNID get_block_at(glm::i32vec3 offset);
    void set_block_at(glm::i32vec3 offset, BlockNID block_nid);
};
