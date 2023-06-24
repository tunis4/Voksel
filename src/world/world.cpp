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

        m_cellular_caves_noise = FastNoise::NewFromEncodedNodeTree("EwCamZk+GgABEQACAAAAAADgQBAAAACIQR8AFgABAAAACwADAAAAAgAAAAMAAAAEAAAAAAAAAD8BFAD//wAAAAAAAD8AAAAAPwAAAAA/AAAAAD8BFwAAAIC/AACAPz0KF0BSuB5AEwAAAKBABgAAj8J1PACamZk+AAAAAAAA4XoUPw==");
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

        std::array<f32, Chunk::volume> cave_noise_output;
        if (world_y <= 32)
            m_cellular_caves_noise->GenUniformGrid3D(cave_noise_output.data(), world_x, world_y, world_z, Chunk::size, Chunk::size, Chunk::size, 0.01f, m_seed);

        for (i32 x = 0; x < Chunk::size; x++) {
            for (i32 z = 0; z < Chunk::size; z++) {
                i32 height = std::floor(noise_output[z * Chunk::size + x] * 32.0f);
                for (i32 y = 0; y < Chunk::size; y++) {
                    block::NID block = 0;
                    if (world_y > 1) {
                        if (world_y == height)
                            block = 4;
                        if (world_y == height - 1)
                            block = 3;
                    } else {
                        if (world_y == height || world_y == height - 1)
                            block = 6;
                        if (world_y <= 0 && world_y > height)
                            block = 9;
                    }
                    if (world_y <= height) {
                        if (world_y > 32 && world_y < height - 1) {
                            block = 1;
                        } else {
                            f32 density = cave_noise_output[z * Chunk::size * Chunk::size + y * Chunk::size + x] * 32.0f;
                            if (world_y < height - 1 && density < 0.1f)
                                block = 1;
                            else if (density >= 0.1f)
                                block = 0;
                        }
                    }
                    // if (world_y < height - 1)
                    //     block = 1;
                    
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

        // // using floor is actually very important, the implicit int-casting will round up for negative numbers
        // glm::i32vec3 current_block_pos(std::floor(start_pos.x / 2.0f), std::floor(start_pos.y / 2.0f), std::floor(start_pos.z / 2.0f));
        // glm::i32vec3 last_block_pos(std::floor(end_pos.x / 2.0f), std::floor(end_pos.y / 2.0f), std::floor(end_pos.z / 2.0f));

        // glm::vec3 ray = glm::normalize(end_pos - start_pos);
        // glm::i32vec3 block_pos_step((ray.x >= 0) ? 1 : -1, (ray.y >= 0) ? 1 : -1, (ray.z >= 0) ? 1 : -1);

        // glm::i32vec3 next_block_pos = current_block_pos + block_pos_step;
        // // Distance along the ray to the next voxel border from the current position (tMaxX, tMaxY, tMaxZ).
        // glm::vec3 next_block_boundary(next_block_pos.x * 2.0f, next_block_pos.y * 2.0f, next_block_pos.z * 2.0f);

        // if (sideDist.x < sideDist.y) {
        //     if (sideDist.x < sideDist.z) {
        //         sideDist.x += deltaDist.x;
        //         mapPos.x += rayStep.x;
        //     } else {
        //         sideDist.z += deltaDist.z;
        //         mapPos.z += rayStep.z;
        //     }
        // } else {
        //     if (sideDist.y < sideDist.z) {
        //         sideDist.y += deltaDist.y;
        //         mapPos.y += rayStep.y;
        //     } else {
        //         sideDist.z += deltaDist.z;
        //         mapPos.z += rayStep.z;
        //     }
        // }

        return result;
    }
}
