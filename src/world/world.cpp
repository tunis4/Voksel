#include "world.hpp"

World::World() {

}

World::~World() {
    
}

std::shared_ptr<Region> World::get_region(glm::i32vec3 region_pos) {
    auto &region = m_regions[region_pos];
    if (!region) region = std::make_shared<Region>();
    return region;
}

std::shared_ptr<Chunk> World::get_chunk(glm::i32vec3 chunk_pos) {
    auto [d, r] = signed_i32vec3_divide(chunk_pos, Region::size);
    auto &chunk = get_region(d)->get_chunk(r);
    if (!chunk) chunk = std::make_shared<Chunk>();
    return chunk;
}

BlockNID World::get_block_at(glm::i32vec3 pos) {
    auto [d, r] = signed_i32vec3_divide(pos, Chunk::size);
    return get_chunk(d)->get_block_at(r);
}

void World::set_block_at(glm::i32vec3 pos, BlockNID block_nid) {
    auto [d, r] = signed_i32vec3_divide(pos, Chunk::size);
    get_chunk(d)->set_block_at(r, block_nid);
}
