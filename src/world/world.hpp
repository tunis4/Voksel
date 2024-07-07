#pragma once

#include <unordered_map>
#include <queue>
#include <shared_mutex>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <FastNoise/FastNoise.h>
#include <moodycamel/concurrentqueue.h>

#include "chunk.hpp"

class World {
public:
    World();
    ~World();

    Chunk* get_chunk(i32vec3 chunk_pos);
    Chunk* get_or_queue_chunk(i32vec3 chunk_pos);
    Chunk* get_or_queue_chunk_with_mesh(i32vec3 chunk_pos);
    void set_chunk(i32vec3 chunk_pos, Chunk *chunk);

    BlockNID get_block_at(i32vec3 pos);
    void set_block_at(i32vec3 pos, BlockNID block_nid);
    u16 get_light_at(i32vec3 pos);
    void set_light_at(i32vec3 pos, u16 light);

    void add_light(i32vec3 pos, u16 light);
    void remove_light(i32vec3 pos);
    
    void mark_chunk_dirty(Chunk *chunk);
    void mark_chunk_mesh_dirty(i32vec3 chunk_pos);

    void shape_chunks();
    void decorate_chunks();
    void apply_changes();
    void mesh_chunks();
    void check_chunks();
    void propagate_light();

    struct RayCastResult {
        bool hit;
        i32vec3 block_pos;
        i32vec3 block_normal;
    };

    // Casts a continuous ray through the world using the DDA method (max distance 32 blocks)
    RayCastResult cast_ray(vec3 start_pos, vec3 end_pos);
    bool is_position_valid(vec3 pos, vec3 min_bounds, vec3 max_bounds);

private:
    i32 m_seed;

    std::unordered_map<i32vec3, Chunk*> m_chunk_map;
    std::shared_mutex m_chunk_map_mutex;

    ConcurrentQueue<i32vec3> m_dirty_chunks;
    ConcurrentQueue<i32vec3> m_chunks_to_shape;
    ConcurrentQueue<i32vec3> m_chunks_to_decorate;
    ConcurrentQueue<i32vec3> m_chunks_to_mesh;
    ConcurrentQueue<i32vec3> m_chunks_failed_to_mesh;
    ConcurrentQueue<i32vec3> m_loaded_chunks;
    ConcurrentQueue<i32vec3> m_checked_chunks;
    ConcurrentQueue<i32vec3> m_chunks_to_save;

    FastNoise::SmartNode<FastNoise::Simplex> m_ridged_simplex_noise;
    FastNoise::SmartNode<FastNoise::FractalRidged> m_ridged_noise;
    FastNoise::SmartNode<FastNoise::Simplex> m_simplex_noise;
    FastNoise::SmartNode<FastNoise::FractalFBm> m_fractal_noise;
    FastNoise::SmartNode<FastNoise::MaxSmooth> m_max_smooth;
    FastNoise::SmartNode<FastNoise::Simplex> m_cave_noise;

    std::queue<i32vec3> m_light_propagation_queue;
    std::queue<std::pair<i32vec3, u16>> m_light_removal_queue;

    void generate_noise(std::array<f32, Chunk::area> &out, i32 x_start, i32 z_start);
    void shape_chunk(i32vec3 chunk_pos);
    usize pos_hash(i32vec3 world_pos);
    void generate_tree(i32vec3 world_pos);
    void decorate_chunk(i32vec3 chunk_pos);
};
