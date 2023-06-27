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

        m_cave_noise = FastNoise::NewFromEncodedNodeTree("HgAIAAETAM3MzD4HAA==");
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
        if (world_y <= 32) {
            m_cave_noise->GenUniformGrid3D(cave_noise_output.data(), world_x, world_y, world_z, Chunk::size, Chunk::size, Chunk::size, 0.019f, m_seed);
        }

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
                            block = 7;
                        if (world_y <= 0 && world_y > height)
                            block = 10;
                    }
                    if (world_y <= height) {
                        if (world_y > 32 && world_y < height - 1) {
                            block = 1;
                        } else {
                            f32 cave_multiplier = 50.0f - 25.0f * std::clamp((world_y + 32.0f) / 64.0f, 0.0f, 1.0f);
                            f32 cave_density = (cave_noise_output[z * Chunk::size * Chunk::size + y * Chunk::size + x] + 1.0f) * cave_multiplier;
                            if (world_y < height - 1 && cave_density < 75.0f) {
                                block = 1;
                            } else if (cave_density >= 75.0f)
                                block = 0;
                        }
                    }
                    
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

    RayCastResult World::cast_ray(glm::vec3 start_pos, glm::vec3 end_pos) {
        RayCastResult result {};

        glm::i32vec3 current_block_pos(glm::floor(start_pos));
        glm::i32vec3 last_block_pos(glm::floor(end_pos));

        glm::vec3 ray = glm::normalize(end_pos - start_pos);
        glm::i32vec3 block_pos_step((ray.x >= 0) ? 1 : -1, (ray.y >= 0) ? 1 : -1, (ray.z >= 0) ? 1 : -1);

        glm::vec3 next_block_boundary(current_block_pos + block_pos_step);
        glm::vec3 t_delta = glm::abs(1.0f / ray);
        glm::vec3 dist(
            (ray.x >= 0) ? (current_block_pos.x + 1 - start_pos.x) : (start_pos.x - current_block_pos.x),
            (ray.y >= 0) ? (current_block_pos.y + 1 - start_pos.y) : (start_pos.y - current_block_pos.y),
            (ray.z >= 0) ? (current_block_pos.z + 1 - start_pos.z) : (start_pos.z - current_block_pos.z)
        );
        glm::vec3 t_max = t_delta * dist;

        i32 stepped_index = -1;
        uint safeguard = 32;
        while (current_block_pos != last_block_pos) {
            if (t_max.x < t_max.y) {
                if (t_max.x < t_max.z) {
                    t_max.x += t_delta.x;
                    current_block_pos.x += block_pos_step.x;
                    stepped_index = 0;
                } else {
                    t_max.z += t_delta.z;
                    current_block_pos.z += block_pos_step.z;
                    stepped_index = 2;
                }
            } else {
                if (t_max.y < t_max.z) {
                    t_max.y += t_delta.y;
                    current_block_pos.y += block_pos_step.y;
                    stepped_index = 1;
                } else {
                    t_max.z += t_delta.z;
                    current_block_pos.z += block_pos_step.z;
                    stepped_index = 2;
                }
            }
            if (get_block_at(current_block_pos) != 0) {
                result.hit = true;
                result.block_pos = current_block_pos;
                if (stepped_index != -1)
                    result.block_normal[stepped_index] = -block_pos_step[stepped_index];
                break;
            }
            if (safeguard == 0) break;
            safeguard--; // this kinda sucks but i cant figure out why the issue is actually happening
        }

        return result;
    }
}
