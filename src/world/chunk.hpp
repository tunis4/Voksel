#pragma once

#include <moodycamel/concurrentqueue.h>
#include <glm/ext/vector_int3_sized.hpp>

#include "block_storage.hpp"
#include "../block/block.hpp"
#include "../renderer/chunk_mesh.hpp"

struct BlockChange {
    uint index;
    BlockNID block_nid;
};

class Chunk {
public:
    static constexpr i32 size = 16;
    static constexpr i32 area = size * size;
    static constexpr i32 volume = size * size * size;

    i32vec3 m_chunk_pos;
    BlockStorage m_storage;
    u16 m_light_map[volume];
    u8 m_sunlight_map[volume / 2];
    ConcurrentQueue<BlockChange> m_blocks_to_change;
    renderer::ChunkMesh *m_mesh = nullptr;

    std::atomic_flag m_decorated = ATOMIC_FLAG_INIT;
    std::atomic_flag m_will_mesh = ATOMIC_FLAG_INIT;
    std::atomic_flag m_dirty = ATOMIC_FLAG_INIT;
    std::atomic_flag m_mesh_dirty = ATOMIC_FLAG_INIT;
    std::atomic_flag m_will_save = ATOMIC_FLAG_INIT;

    Chunk(i32vec3 chunk_pos);
    ~Chunk();
    Chunk(const Chunk &other) = delete;

    void change_block_at(i32vec3 offset, BlockNID block_nid);
    void apply_changes();

    BlockNID get_block_at(i32vec3 offset);
    void set_block(uint index, BlockNID block_nid);
    void set_block_at(i32vec3 offset, BlockNID block_nid);

    u16 get_light_at(i32vec3 offset);
    void set_light_at(i32vec3 offset, u16 light);

    SerialBuffer& serialize(SerialBuffer &buffer);
    SerialBuffer& deserialize(SerialBuffer &buffer);
};
