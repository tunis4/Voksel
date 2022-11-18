#pragma once

#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "region.hpp"

class World {
    std::unordered_map<glm::i32vec3, std::shared_ptr<Region>> m_regions;

public:
    World();
    ~World();

    std::shared_ptr<Region> get_region(glm::i32vec3 region_pos);
    std::shared_ptr<Chunk> get_chunk(glm::i32vec3 chunk_pos);
    std::shared_ptr<Chunk> get_or_generate_chunk(glm::i32vec3 chunk_pos);
    BlockNID get_block_at(glm::i32vec3 pos);
    void set_block_at(glm::i32vec3 pos, BlockNID block_nid);
};
