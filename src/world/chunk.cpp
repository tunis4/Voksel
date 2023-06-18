#include "chunk.hpp"

#include <stdexcept>

namespace world {
    Chunk::Chunk() {}
    Chunk::~Chunk() {}

    block::NID Chunk::get_block_at(glm::i32vec3 offset) {
        if (offset.x < 0 || offset.x > size - 1 || offset.y < 0 || offset.y > size - 1 || offset.z < 0 || offset.z > size - 1)
            throw std::out_of_range("Block offset is out of chunk's range");
        
        return m_blocks[util::coords_to_index<Chunk::size>(offset.x, offset.y, offset.z)];
    }

    void Chunk::set_block_at(glm::i32vec3 offset, block::NID block_nid) {
        if (offset.x < 0 || offset.x > size - 1 || offset.y < 0 || offset.y > size - 1 || offset.z < 0 || offset.z > size - 1)
            throw std::out_of_range("Block offset is out of chunk's range");
        
        m_blocks[util::coords_to_index<Chunk::size>(offset.x, offset.y, offset.z)] = block_nid;
    }
}
