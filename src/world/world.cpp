#include "world.hpp"
#include "chunk.hpp"

World::World() {
    u64 seed = 0;

    m_flat_base_terrain.SetFrequency(2);

    m_flat_terrain.SetSourceModule(0, m_flat_base_terrain);
    m_flat_terrain.SetScale(0.125);
    m_flat_terrain.SetBias(-0.75);

    m_terrain_type.SetFrequency(0.5);
    m_terrain_type.SetPersistence(0.25);

    m_terrain_selector.SetSourceModule(0, m_flat_terrain);
    m_terrain_selector.SetSourceModule(1, m_mountain_terrain);
    m_terrain_selector.SetControlModule(m_terrain_type);
    m_terrain_selector.SetBounds(0, 1000);
    m_terrain_selector.SetEdgeFalloff(0.125);

    m_final_terrain.SetSourceModule(0, m_terrain_selector);
    m_final_terrain.SetFrequency(4);
    m_final_terrain.SetPower(0.125);
}

World::~World() {
    
}

std::shared_ptr<Chunk> World::get_chunk(glm::i32vec3 chunk_pos) {
    return m_chunks[chunk_pos];
}

std::shared_ptr<Chunk> World::get_or_generate_chunk(glm::i32vec3 chunk_pos) {
    auto &chunk = m_chunks[chunk_pos];
    if (!chunk) chunk = generate_chunk(chunk_pos);
    return chunk;
}

std::shared_ptr<Chunk> World::generate_chunk(glm::i32vec3 chunk_pos) {
    auto chunk = std::make_shared<Chunk>();

    auto world_x = chunk_pos.x * Chunk::size;
    auto world_y = chunk_pos.y * Chunk::size;
    auto world_z = chunk_pos.z * Chunk::size;
    for (i32 x = 0; x < Chunk::size; x++) {
        for (i32 z = 0; z < Chunk::size; z++) {
            i32 height = floor(m_final_terrain.GetValue(world_x / 256.0, 0, world_z / 256.0) * 32.0);
            for (i32 y = 0; y < Chunk::size; y++) {
                block::NID block = 0;
                if (world_y == height)
                    block = 4;
                if (world_y == height - 1)
                    block = 3;
                if (world_y < height - 1)
                    block = 1;
                chunk->m_blocks[util::coords_to_index<Chunk::size>(x, y, z)] = block;
                world_y++;
            }
            world_y -= Chunk::size;
            world_z++;
        }
        world_z -= Chunk::size;
        world_x++;
    }

    return chunk;
}

block::NID World::get_block_at(glm::i32vec3 pos) {
    auto [d, r] = util::signed_i32vec3_divide(pos, Chunk::size);
    return get_or_generate_chunk(d)->get_block_at(r);
}

void World::set_block_at(glm::i32vec3 pos, block::NID block_nid) {
    auto [d, r] = util::signed_i32vec3_divide(pos, Chunk::size);
    get_or_generate_chunk(d)->set_block_at(r, block_nid);
}
