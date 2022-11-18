#include "region.hpp"

#include <stdexcept>

Region::Region() {

}

Region::~Region() {

}

std::shared_ptr<Chunk>& Region::get_chunk(glm::i32vec3 offset) {
    if (offset.x < 0 || offset.x > size - 1 || offset.y < 0 || offset.y > size - 1 || offset.z < 0 || offset.z > size - 1)
        throw std::out_of_range("Chunk offset is out of region's range");
    
    return m_chunks[offset.x * size * size + offset.y * size + offset.z];
}
