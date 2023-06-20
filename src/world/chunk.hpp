#pragma once

#include <memory>
#include <glm/ext/vector_int3_sized.hpp>

#include "block_storage.hpp"
#include "../block/block.hpp"

namespace world {
    class Chunk {
    public:
        static constexpr i32 size = 32;
        static constexpr i32 area = size * size;
        static constexpr i32 volume = size * size * size;

        BlockStorage m_storage;

        Chunk();
        ~Chunk();

        block::NID get_block_at(glm::i32vec3 offset);
        void set_block_at(glm::i32vec3 offset, block::NID block_nid);
    };
}
