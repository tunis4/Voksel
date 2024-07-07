#include "chunk.hpp"
#include "../game.hpp"
#include <lz4.h>

Chunk::Chunk(i32vec3 chunk_pos) : m_chunk_pos(chunk_pos), m_storage(Chunk::volume) {
    std::memset(m_light_map, ~(u16)0, sizeof(m_light_map));
}
Chunk::~Chunk() {}

void Chunk::change_block_at(i32vec3 offset, BlockNID block_nid) {
    uint index = coords_to_index<Chunk::size>(offset.x, offset.y, offset.z);
    m_blocks_to_change.enqueue(BlockChange(index, block_nid));
    auto world = Game::get()->world();
    world->mark_chunk_dirty(this);
    if (offset.x == 0) world->mark_chunk_mesh_dirty(m_chunk_pos + i32vec3(-1, 0, 0));
    else if (offset.x == Chunk::size - 1) world->mark_chunk_mesh_dirty(m_chunk_pos + i32vec3(1, 0, 0));
    if (offset.y == 0) world->mark_chunk_mesh_dirty(m_chunk_pos + i32vec3(0, -1, 0));
    else if (offset.y == Chunk::size - 1) world->mark_chunk_mesh_dirty(m_chunk_pos + i32vec3(0, 1, 0));
    if (offset.z == 0) world->mark_chunk_mesh_dirty(m_chunk_pos + i32vec3(0, 0, -1));
    else if (offset.z == Chunk::size - 1) world->mark_chunk_mesh_dirty(m_chunk_pos + i32vec3(0, 0, 1));
}

void Chunk::apply_changes() {
    BlockChange block_changes[16];
    while (usize num = m_blocks_to_change.try_dequeue_bulk(block_changes, 16))
        for (usize i = 0; i < num; i++)
            set_block(block_changes[i].index, block_changes[i].block_nid);
}

BlockNID Chunk::get_block_at(i32vec3 offset) {
    return m_storage.get_block(coords_to_index<Chunk::size>(offset.x, offset.y, offset.z));
}

void Chunk::set_block(uint index, BlockNID block_nid) {
    if (get_block_data(block_nid)->m_top_transparent)
        m_light_map[index] = ~(u16)0; // set full light
    else
        m_light_map[index] = (u16)0; // set no light

    m_storage.set_block(index, block_nid);
}

void Chunk::set_block_at(i32vec3 offset, BlockNID block_nid) {
    uint index = coords_to_index<Chunk::size>(offset.x, offset.y, offset.z);
    set_block(index, block_nid);
}

u16 Chunk::get_light_at(i32vec3 offset) {
    return m_light_map[coords_to_index<Chunk::size>(offset.x, offset.y, offset.z)];
}

void Chunk::set_light_at(i32vec3 offset, u16 light) {
    m_light_map[coords_to_index<Chunk::size>(offset.x, offset.y, offset.z)] = light;
}

SerialBuffer& Chunk::serialize(SerialBuffer &buffer) {
    SerialBuffer original {};
    m_storage.serialize(original);
    original.push(m_light_map, sizeof(m_light_map));
    original.push(m_sunlight_map, sizeof(m_sunlight_map));

    auto compressed = original.compress();
    buffer.push(compressed.data(), compressed.size());
    buffer.push((int)compressed.size());
    buffer.push((int)original.size());
    return buffer;
}

SerialBuffer& Chunk::deserialize(SerialBuffer &buffer) {
    int original_size = 0, compressed_size = 0;
    buffer.pop(original_size);
    buffer.pop(compressed_size);
    std::vector<u8> compressed(compressed_size);
    buffer.pop(compressed.data(), compressed_size);

    SerialBuffer decompressed = SerialBuffer::decompress(compressed, original_size);
    decompressed.pop(m_sunlight_map, sizeof(m_sunlight_map));
    decompressed.pop(m_light_map, sizeof(m_light_map));
    m_storage.deserialize(decompressed);
    return buffer;
}
