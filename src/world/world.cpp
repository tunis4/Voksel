#include "world.hpp"
#include "chunk.hpp"

namespace world {
    World::World() {
        m_seed = 1337;

        m_simplex_noise = FastNoise::New<FastNoise::Simplex>();

        m_ridged_simplex_noise = FastNoise::New<FastNoise::Simplex>();
        m_ridged_noise = FastNoise::New<FastNoise::FractalRidged>();
        m_ridged_noise->SetSource(m_ridged_simplex_noise);
        m_ridged_noise->SetOctaveCount(3);

        m_fractal_noise = FastNoise::New<FastNoise::FractalFBm>();
        m_fractal_noise->SetSource(m_simplex_noise);
        m_fractal_noise->SetOctaveCount(3);

        m_max_smooth = FastNoise::New<FastNoise::MaxSmooth>();
        m_max_smooth->SetLHS(m_ridged_noise);
        m_max_smooth->SetRHS(m_fractal_noise);
    }

    World::~World() {}

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

        std::array<f32, Chunk::area> noise_output;
        m_max_smooth->GenUniformGrid2D(noise_output.data(), world_x, world_z, Chunk::size, Chunk::size, 0.005f, m_seed);

        for (i32 x = 0; x < Chunk::size; x++) {
            for (i32 z = 0; z < Chunk::size; z++) {
                i32 height = std::floor(noise_output[z * Chunk::size + x] * 32.0f);
                for (i32 y = 0; y < Chunk::size; y++) {
                    block::NID block = 0;
                    if (world_y > -23) {
                        if (world_y == height)
                            block = 4;
                        if (world_y == height - 1)
                            block = 3;
                    } else {
                        if (world_y == height)
                            block = 6;
                        if (world_y == height - 1)
                            block = 6;
                    }
                    if (world_y < height - 1)
                        block = 1;
                    chunk->m_storage.set_block(util::coords_to_index<Chunk::size>(x, y, z), block);
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

    RayCastResult cast_ray(glm::vec3 start_pos, glm::vec3 end_pos) {
        RayCastResult result {};
        glm::vec3 ray = glm::normalize(end_pos - start_pos);
        
        return result;
    }
}
