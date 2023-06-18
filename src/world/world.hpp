#pragma once

#include "FastNoise/Generators/Fractal.h"
#include "FastNoise/Generators/Simplex.h"
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <FastNoise/FastNoise.h>

#include "chunk.hpp"

namespace world {
    class World {
        u64 m_seed;
        FastNoise::SmartNode<FastNoise::Simplex> m_ridged_simplex_noise;
        FastNoise::SmartNode<FastNoise::FractalRidged> m_ridged_noise;
        FastNoise::SmartNode<FastNoise::Simplex> m_simplex_noise;
        FastNoise::SmartNode<FastNoise::FractalFBm> m_fractal_noise;
        FastNoise::SmartNode<FastNoise::MaxSmooth> m_max_smooth;
        // noise::module::RidgedMulti m_mountain_terrain;
        // noise::module::Billow m_flat_base_terrain;
        // noise::module::ScaleBias m_flat_terrain;
        // noise::module::Perlin m_terrain_type;
        // noise::module::Select m_terrain_selector;
        // noise::module::Turbulence m_final_terrain;
        
        std::unordered_map<glm::i32vec3, std::shared_ptr<Chunk>> m_chunks;

    public:
        World();
        ~World();

        std::shared_ptr<Chunk> get_chunk(glm::i32vec3 chunk_pos);
        std::shared_ptr<Chunk> generate_chunk(glm::i32vec3 chunk_pos);
        std::shared_ptr<Chunk> get_or_generate_chunk(glm::i32vec3 chunk_pos);
        block::NID get_block_at(glm::i32vec3 pos);
        void set_block_at(glm::i32vec3 pos, block::NID block_nid);
    };
}
