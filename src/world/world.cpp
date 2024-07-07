#include "world.hpp"
#include "chunk.hpp"
#include "../parallel_executor.hpp"
#include "../renderer/chunk_renderer.hpp"

#include <filesystem>
#include <fstream>
#include <entt/entt.hpp>

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

    m_cave_noise = FastNoise::New<FastNoise::Simplex>();

    // std::filesystem::path world_dir("worlds");
    // world_dir /= "testing";
    // log(LogLevel::INFO, "World", "Loading world from {}...", world_dir);
    // if (std::filesystem::is_regular_file(world_dir / "chunks")) {
    //     std::ifstream input(world_dir / "chunks", std::ios::binary);
    //     usize original_size = 0;
    //     input.read((char*)&original_size, sizeof(original_size));
    //     std::vector<u8> compressed(std::istreambuf_iterator<char>(input), {});
    //     SerialBuffer buffer = SerialBuffer::decompress(compressed, original_size);
    //     int num_chunks = 0;
    //     buffer.pop(num_chunks);
    //     for (int i = 0; i < num_chunks; i++) {
    //         i32vec3 chunk_pos;
    //         buffer.pop(chunk_pos);
    //         Chunk *chunk = new Chunk(chunk_pos);
    //         chunk->deserialize(buffer);
    //         m_chunk_map[chunk_pos] = chunk;
    //         chunk->m_decorated.test_and_set();
    //         chunk->m_will_save.test_and_set();
    //         m_chunks_to_save.enqueue(chunk);
    //         mark_chunk_mesh_dirty(chunk_pos);
    //     }
    //     log(LogLevel::INFO, "World", "World successfully loaded");
    // } else {
    //     log(LogLevel::ERROR, "World", "Failed to load world due to it not existing");
    //     log(LogLevel::WARN, "World", "Creating new world instead");
    // }
}

World::~World() {
    // log(LogLevel::INFO, "World", "Saving world...");
    // std::filesystem::path world_dir("worlds");
    // world_dir /= "testing";
    // std::filesystem::create_directories(world_dir);
    // std::ofstream output(world_dir / "chunks", std::ios::binary | std::ios::trunc);

    // SerialBuffer buffer;
    // int num_chunks = 0;
    // Chunk *chunk;
    // while (m_chunks_to_save.try_dequeue(chunk)) {
    //     if (!chunk)
    //         continue;
    //     chunk->serialize(buffer);
    //     buffer.push(chunk->m_chunk_pos);
    //     num_chunks++;
    // }
    // buffer.push(num_chunks);
    // usize original_size = buffer.size();
    // output.write((const char*)&original_size, sizeof(original_size));
    // auto compressed = buffer.compress();
    // output.write((const char*)compressed.data(), compressed.size());
    // output.close();

    // log(LogLevel::INFO, "World", "Saved world to {} ({} chunks -> {} MiB)", world_dir, num_chunks, ((double)compressed.size() / 1024.0) / 1024.0);

    for (auto [chunk_pos, chunk] : m_chunk_map)
        delete chunk;
}

static thread_local i32vec3 last_chunk_pos = i32vec3(INT_MAX);
static thread_local Chunk *last_chunk = nullptr;

Chunk* World::get_chunk(i32vec3 chunk_pos) {
    if (chunk_pos == last_chunk_pos) [[likely]]
        return last_chunk;
    last_chunk_pos = chunk_pos;

    std::shared_lock guard(m_chunk_map_mutex);
    auto it = m_chunk_map.find(chunk_pos);
    Chunk *chunk = nullptr;
    if (it != m_chunk_map.end())
        chunk = it->second;
    last_chunk = chunk;
    return chunk;
}

Chunk* World::get_or_queue_chunk(i32vec3 chunk_pos) {
    if (chunk_pos == last_chunk_pos && last_chunk) [[likely]]
        return last_chunk;
    last_chunk_pos = chunk_pos;

    std::scoped_lock guard(m_chunk_map_mutex);
    auto it = m_chunk_map.find(chunk_pos);
    if (it != m_chunk_map.end() && it->second) {
        last_chunk = it->second;
    } else {
        auto chunk = new Chunk(chunk_pos);
        m_chunk_map[chunk_pos] = chunk;
        m_chunks_to_shape.enqueue(chunk_pos);
        last_chunk = chunk;
    }
    return last_chunk;
}
static std::mutex print_mutex;

Chunk* World::get_or_queue_chunk_with_mesh(i32vec3 chunk_pos) {
    auto chunk = get_or_queue_chunk(chunk_pos);
    if (chunk->m_will_mesh.test_and_set() == false)
        mark_chunk_mesh_dirty(chunk_pos);
    return chunk;
}

void World::mark_chunk_dirty(Chunk *chunk) {
    if (!chunk)
        return;
    if (chunk->m_dirty.test_and_set())
        return; // already dirty
    m_dirty_chunks.enqueue(chunk->m_chunk_pos);
}

void World::mark_chunk_mesh_dirty(i32vec3 chunk_pos) {
    auto chunk = get_chunk(chunk_pos);
    if (!chunk || !chunk->m_will_mesh.test())
        return;
    if (chunk->m_mesh_dirty.test_and_set())
        return; // already dirty
    m_chunks_to_mesh.enqueue(chunk_pos);
            // {
            //     std::scoped_lock guard(print_mutex);
            //     std::cout << "queued " << chunk_pos << " " << chunk << std::endl;
            // }
}

BlockNID World::get_block_at(i32vec3 pos) {
    auto [chunk_pos, chunk_offset] = signed_i32vec3_divide(pos, Chunk::size);
    auto chunk = get_or_queue_chunk(chunk_pos);
    return chunk->get_block_at(chunk_offset);
}

void World::set_block_at(i32vec3 pos, BlockNID block_nid) {
    auto [chunk_pos, chunk_offset] = signed_i32vec3_divide(pos, Chunk::size);
    auto chunk = get_or_queue_chunk(chunk_pos);
    chunk->change_block_at(chunk_offset, block_nid);

    if (!chunk->m_will_save.test_and_set())
        m_chunks_to_save.enqueue(chunk_pos);
}

u16 World::get_light_at(i32vec3 pos) {
    auto [chunk_pos, chunk_offset] = signed_i32vec3_divide(pos, Chunk::size);
    return get_or_queue_chunk(chunk_pos)->get_light_at(chunk_offset);
}

void World::set_light_at(i32vec3 pos, u16 light) {
    auto [chunk_pos, chunk_offset] = signed_i32vec3_divide(pos, Chunk::size);
    auto chunk = get_or_queue_chunk(chunk_pos);
    chunk->set_light_at(chunk_offset, light);

    mark_chunk_mesh_dirty(chunk_pos);
    if (chunk_offset.x == 0) mark_chunk_mesh_dirty(chunk_pos + i32vec3(-1, 0, 0));
    else if (chunk_offset.x == Chunk::size - 1) mark_chunk_mesh_dirty(chunk_pos + i32vec3(1, 0, 0));
    if (chunk_offset.y == 0) mark_chunk_mesh_dirty(chunk_pos + i32vec3(0, -1, 0));
    else if (chunk_offset.y == Chunk::size - 1) mark_chunk_mesh_dirty(chunk_pos + i32vec3(0, 1, 0));
    if (chunk_offset.z == 0) mark_chunk_mesh_dirty(chunk_pos + i32vec3(0, 0, -1));
    else if (chunk_offset.z == Chunk::size - 1) mark_chunk_mesh_dirty(chunk_pos + i32vec3(0, 0, 1));
}

World::RayCastResult World::cast_ray(vec3 start_pos, vec3 end_pos) {
    RayCastResult result {};

    i32vec3 current_block_pos(glm::floor(start_pos));
    i32vec3 last_block_pos(glm::floor(end_pos));

    vec3 ray = glm::normalize(end_pos - start_pos);

    i32vec3 block_pos_step((ray.x >= 0) ? 1 : -1, (ray.y >= 0) ? 1 : -1, (ray.z >= 0) ? 1 : -1);

    vec3 t_delta = glm::abs(1.0f / ray);
    vec3 dist(
        (ray.x >= 0) ? (current_block_pos.x + 1 - start_pos.x) : (start_pos.x - current_block_pos.x),
        (ray.y >= 0) ? (current_block_pos.y + 1 - start_pos.y) : (start_pos.y - current_block_pos.y),
        (ray.z >= 0) ? (current_block_pos.z + 1 - start_pos.z) : (start_pos.z - current_block_pos.z)
    );
    vec3 t_max = t_delta * dist;

    i32 stepped_index = -1;
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
        BlockNID current = get_block_at(current_block_pos);
        if (current != 0) {
            result.hit = true;
            result.block_pos = current_block_pos;
            if (stepped_index != -1)
                result.block_normal[stepped_index] = -block_pos_step[stepped_index];
            break;
        }
    }

    return result;
}

bool World::is_position_valid(vec3 pos, vec3 min_bounds, vec3 max_bounds) {
    i32vec3 min_block_pos(glm::floor(min_bounds + pos));
    i32vec3 max_block_pos(glm::floor(max_bounds + pos));
    for (i32 y = min_block_pos.y; y <= max_block_pos.y; y++) {
        for (i32 x = min_block_pos.x; x <= max_block_pos.x; x++) {
            for (i32 z = min_block_pos.z; z <= max_block_pos.z; z++) {
                BlockNID current = get_block_at(i32vec3(x, y, z));
                if (get_block_data(current)->m_collidable)
                    return false;
            }
        }
    }
    return true;
}

inline uint intensity(u16 light) {
    return (light >> 12) & 0xF;
}

inline u16 with_intensity(u16 light, uint intensity) {
    return (light & 0x0FFF) | (intensity << 12);
}

void World::add_light(i32vec3 pos, u16 light) {
    set_light_at(pos, light);
    m_light_propagation_queue.emplace(pos);
    propagate_light();
}

constexpr std::array neighbor_offsets = std::to_array<i32vec3>({
    i32vec3( 0,  1,  0), i32vec3( 0, -1,  0),
    i32vec3( 1,  0,  0), i32vec3(-1,  0,  0),
    i32vec3( 0,  0,  1), i32vec3( 0,  0, -1)
});

void World::remove_light(i32vec3 pos) {
    m_light_removal_queue.emplace(pos, get_light_at(pos));
    set_light_at(pos, 0);
    while (!m_light_removal_queue.empty()) {
        auto [current_pos, current_light] = m_light_removal_queue.front();
        m_light_removal_queue.pop();
        uint current_light_level = intensity(current_light);
        for (i32vec3 neighbor_offset : neighbor_offsets) {
            i32vec3 neighbor_pos = current_pos + neighbor_offset;
            u16 neighbor_light = get_light_at(neighbor_pos);
            uint neighbor_light_level = intensity(neighbor_light);
            if (neighbor_light_level != 0 && neighbor_light_level < current_light_level) {
                set_light_at(neighbor_pos, 0);
                m_light_removal_queue.emplace(neighbor_pos, neighbor_light);
            } else if (neighbor_light_level >= current_light_level) {
                m_light_propagation_queue.emplace(neighbor_pos);
            }
        }
    }
    propagate_light();
}

void World::propagate_light() {
    while (!m_light_propagation_queue.empty()) {
        i32vec3 current_pos = m_light_propagation_queue.front();
        m_light_propagation_queue.pop();
        uint current_light = get_light_at(current_pos);
        uint current_light_level = intensity(current_light);
        for (i32vec3 neighbor_offset : neighbor_offsets) {
            i32vec3 neighbor_pos = current_pos + neighbor_offset;
            if (get_block_data(get_block_at(neighbor_pos))->m_top_transparent && intensity(get_light_at(neighbor_pos)) + 2 <= current_light_level) {
                set_light_at(neighbor_pos, with_intensity(current_light, current_light_level - 1));
                m_light_propagation_queue.emplace(neighbor_pos);
            }
        }
    }
}

static ParallelExecutor executor;
static std::atomic_int num_shaped, num_decorated, num_applied, num_meshed;

void World::shape_chunks() {
    num_shaped = 0;
    executor.run([&] (u8) {
        i64 time_spent = 0;
        i32vec3 chunk_pos;
        while (time_spent < 6000 && m_chunks_to_shape.try_dequeue(chunk_pos)) {
            auto start = std::chrono::steady_clock::now();
            shape_chunk(chunk_pos);
            m_chunks_to_decorate.enqueue(chunk_pos);

            auto stop = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            time_spent += duration.count();
            num_shaped++;
        }
    });
}

void World::decorate_chunks() {
    num_decorated = 0;
    executor.run([&] (u8) {
        i64 time_spent = 0;
        i32vec3 chunk_pos;
        while (time_spent < 6000 && m_chunks_to_decorate.try_dequeue(chunk_pos)) {
            auto start = std::chrono::steady_clock::now();
            decorate_chunk(chunk_pos);
            m_loaded_chunks.enqueue(chunk_pos);
            num_decorated++;

            auto stop = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            time_spent += duration.count();
        }
    });
}

void World::apply_changes() {
    num_applied = 0;
    executor.run([&] (u8) {
        i32vec3 chunk_pos;
        while (m_dirty_chunks.try_dequeue(chunk_pos)) {
            auto chunk = get_chunk(chunk_pos);
            if (!chunk)
                continue;
            chunk->apply_changes();
            mark_chunk_mesh_dirty(chunk->m_chunk_pos);
            chunk->m_dirty.clear();
            num_applied++;
        }
    });
}

void World::mesh_chunks() {
    num_meshed = 0;
    renderer::ChunkRenderer &renderer = entt::locator<renderer::ChunkRenderer>::value();
    executor.run([&] (u8) {
        i64 time_spent = 0;
        i32vec3 chunk_pos;
        while (time_spent < 6000 && m_chunks_to_mesh.try_dequeue(chunk_pos)) {
            auto start = std::chrono::steady_clock::now();
            auto chunk = get_chunk(chunk_pos);
            if (!chunk) {
                {
                    std::scoped_lock guard(print_mutex);
                    std::cout << "voiding 1 " << chunk_pos << std::endl;
                }
                continue;
            }

            renderer::ChunkMeshBuilder *mesh_builder;
            if (!renderer.m_available_mesh_builders.try_dequeue(mesh_builder)) {
                m_chunks_failed_to_mesh.enqueue(chunk_pos);
                break;
            }

            if (mesh_builder->build(chunk)) {
                chunk->m_mesh_dirty.clear();
                renderer.m_finished_mesh_builders.enqueue(mesh_builder);
                num_meshed++;
            } else {
                m_chunks_failed_to_mesh.enqueue(chunk_pos);
                renderer.m_available_mesh_builders.enqueue(mesh_builder);
            }

            auto stop = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            time_spent += duration.count();
        }
    });

    executor.run([&] (u8) {
        i32vec3 chunks[8];
        while (usize num = m_chunks_failed_to_mesh.try_dequeue_bulk(chunks, 8))
            m_chunks_to_mesh.enqueue_bulk(chunks, num);
    });

    // log(LogLevel::INFO, "World", "Shaped {}, Decorated {}, Applied {}, Meshed {} chunks", num_shaped.load(), num_decorated.load(), num_applied.load(), num_meshed.load());
    // log(LogLevel::INFO, "World", "{} to shape, {} to decorate, {} to apply, {} to mesh", m_chunks_to_shape.size_approx(), m_chunks_to_decorate.size_approx(), m_dirty_chunks.size_approx(), m_chunks_to_mesh.size_approx());
}

void World::check_chunks() {
    renderer::ChunkRenderer &renderer = entt::locator<renderer::ChunkRenderer>::value();
    executor.run([&] (u8) {
        i32vec3 chunk_pos;
        while (m_loaded_chunks.try_dequeue(chunk_pos)) {
            auto chunk = get_chunk(chunk_pos);
            if (!chunk) {
                {
                    std::scoped_lock guard(print_mutex);
                    std::cout << "voiding 2 " << chunk_pos << std::endl;
                }
                continue;
            }

            // if (!unload) {
                m_checked_chunks.enqueue(chunk_pos);
            // } else {
            //     if (chunk->m_mesh) {
            //         chunk->m_will_mesh.clear();
            //         chunk->m_mesh_dirty.clear();
            //         renderer.m_meshes_to_free.enqueue(chunk->m_mesh);
            //         chunk->m_mesh = nullptr;
            //     }
            //     {
            //         std::scoped_lock guard(m_chunk_map_mutex);
            //         m_chunk_map[chunk->m_chunk_pos] = nullptr;
            //     }
            //     if (last_chunk == chunk)
            //         last_chunk = nullptr;
            //     delete chunk;
            // }
        }
    });
    executor.run([&] (u8) {
        i32vec3 chunks[8];
        while (usize num = m_checked_chunks.try_dequeue_bulk(chunks, 8))
            m_loaded_chunks.enqueue_bulk(chunks, num);
    });
}
