#include "chunk.hpp"

#include <stdexcept>

Chunk::Chunk() {
    for (i32 x = 0; x < size; x++) {
        for (i32 y = 0; y < size; y++) {
            for (i32 z = 0; z < size; z++) {
                BlockNID *nid = __get_block_at(x, y, z);
                *nid = 1;
            }
        }
    }
}

Chunk::~Chunk() {

}

BlockNID Chunk::get_block_at(glm::i32vec3 offset) {
    if (offset.x < 0 || offset.x > size - 1 || offset.y < 0 || offset.y > size - 1 || offset.z < 0 || offset.z > size - 1)
        throw std::out_of_range("Block offset is out of chunk's range");
    
    return *__get_block_at(offset.x, offset.y, offset.z);
}

void Chunk::set_block_at(glm::i32vec3 offset, BlockNID block_nid) {
    if (offset.x < 0 || offset.x > size - 1 || offset.y < 0 || offset.y > size - 1 || offset.z < 0 || offset.z > size - 1)
        throw std::out_of_range("Block offset is out of chunk's range");
    
    *__get_block_at(offset.x, offset.y, offset.z) = block_nid;
}
