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

        std::mutex m_mutex;
        BlockStorage m_storage;

        u16 m_light_map[volume];
        u8 m_sunlight_map[volume / 2];

        Chunk();
        ~Chunk();
        Chunk(const Chunk &other) = delete;

        block::NID get_block_at(glm::i32vec3 offset);
        void set_block_at(glm::i32vec3 offset, block::NID block_nid);
    };
}
