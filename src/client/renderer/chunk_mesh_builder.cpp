#include "chunk_mesh_builder.hpp"
#include "../client.hpp"
#include "src/world/chunk.hpp"

namespace render {
    ChunkMeshBuilder::ChunkMeshBuilder(ChunkRenderer *renderer) {
        m_renderer = renderer;
    }

    ChunkMeshBuilder::~ChunkMeshBuilder() {}

    void ChunkMeshBuilder::push_face_indices(bool flipped) {
        if (flipped) {
            m_indices.push_back(m_last_index + 1);
            m_indices.push_back(m_last_index + 4);
            m_indices.push_back(m_last_index + 2);
            m_indices.push_back(m_last_index + 1);
            m_indices.push_back(m_last_index + 3);
            m_indices.push_back(m_last_index + 4);
        } else {
            m_indices.push_back(m_last_index + 1);
            m_indices.push_back(m_last_index + 3);
            m_indices.push_back(m_last_index + 2);
            m_indices.push_back(m_last_index + 2);
            m_indices.push_back(m_last_index + 3);
            m_indices.push_back(m_last_index + 4);
        }
        m_last_index += 4;
    }

#define NC(x, y, z) neighbor_chunks[util::coords_to_index<3>(x, y, z)]
#define NB(x, y, z) neighbor_blocks[util::coords_to_index<3>(x, y, z)]
#define V_AO(side1, side2, corner) ((bool)side1 && (bool)side2) ? 0 : 3 - ((bool)side1 + (bool)side2 + (bool)corner)
    void ChunkMeshBuilder::build() {
        world::Chunk *neighbor_chunks[3 * 3 * 3];
        for (i32 y = -1; y < 2; y++)
            for (i32 z = -1; z < 2; z++)
                for (i32 x = -1; x < 2; x++)
                    NC(x + 1, y + 1, z + 1) = client::Client::get()->world()->get_or_generate_chunk((*m_current).m_chunk_pos + glm::i32vec3(x, y, z)).get();

        block::NID neighbor_blocks[3 * 3 * 3]; // neighbor blocks

        auto chunk = NC(1, 1, 1);
        m_vertices.clear();
        m_indices.clear();
        m_last_index = -1;
        
        if (chunk->m_storage.m_bits_per_index == 0 && chunk->m_storage.m_data.single->type == 0)
            return; // the chunk is just void
        
        // FIXME: handle the case where the single type isnt void
        u32 unpacked[world::Chunk::volume];
        PackedArray_unpack(chunk->m_storage.m_data.packed, 0, unpacked, world::Chunk::volume);
        for (usize i = 0; i < world::Chunk::volume; i++)
            unpacked[i] = chunk->m_storage.m_palette[unpacked[i]].type;

        for (usize y = 0; y < world::Chunk::size; y++) {
            for (usize z = 0; z < world::Chunk::size; z++) {
                for (usize x = 0; x < world::Chunk::size; x++) {
                    block::NID current_block = unpacked[util::coords_to_index<world::Chunk::size>(x, y, z)];
                    if (current_block == 0) // guaranteed to be void
                        continue;

                    block::BlockData *data = block::get_block_data(current_block);

                    for (i32 nx = -1; nx < 2; nx++) {
                        for (i32 ny = -1; ny < 2; ny++) {
                            for (i32 nz = -1; nz < 2; nz++) {
                                i32 cx = 1, cy = 1, cz = 1, rx = x + nx, ry = y + ny, rz = z + nz;
                                if (rx == -1) { cx = 0; rx = world::Chunk::size - 1; }
                                else if (rx == world::Chunk::size) { cx = 2; rx = 0; }
                                if (ry == -1) { cy = 0; ry = world::Chunk::size - 1; }
                                else if (ry == world::Chunk::size) { cy = 2; ry = 0; }
                                if (rz == -1) { cz = 0; rz = world::Chunk::size - 1; } 
                                else if (rz == world::Chunk::size) { cz = 2; rz = 0; }
                                auto c = NC(cx, cy, cz);
                                NB(nx + 1, ny + 1, nz + 1) = c->m_storage.get_block(util::coords_to_index<world::Chunk::size>(rx, ry, rz));
                            }
                        }
                    }

                    bool top_visible = NB(1, 2, 1) == 0;
                    bool bottom_visible = NB(1, 0, 1) == 0;
                    bool north_visible = NB(2, 1, 1) == 0;
                    bool south_visible = NB(0, 1, 1) == 0;
                    bool west_visible = NB(1, 1, 0) == 0;
                    bool east_visible = NB(1, 1, 2) == 0;
                
                    auto pos = glm::vec3(x * 2, y * 2, z * 2);
                    if (top_visible) {
                        u8 ao0 = V_AO(NB(0, 2, 1), NB(1, 2, 0), NB(0, 2, 0)), ao1 = V_AO(NB(0, 2, 1), NB(1, 2, 2), NB(0, 2, 2));
                        u8 ao2 = V_AO(NB(1, 2, 0), NB(2, 2, 1), NB(2, 2, 0)), ao3 = V_AO(NB(1, 2, 2), NB(2, 2, 1), NB(2, 2, 2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1, -1), 1, data->m_top_tex_layer, ao0));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1,  1), 3, data->m_top_tex_layer, ao1));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1, -1), 0, data->m_top_tex_layer, ao2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1,  1), 2, data->m_top_tex_layer, ao3));
                        push_face_indices(ao0 + ao3 > ao1 + ao2);
                    }
                    if (bottom_visible) {
                        u8 ao0 = V_AO(NB(0, 0, 1), NB(1, 0, 0), NB(0, 0, 0)), ao1 = V_AO(NB(2, 0, 1), NB(1, 0, 0), NB(2, 0, 0));
                        u8 ao2 = V_AO(NB(0, 0, 1), NB(1, 0, 2), NB(0, 0, 2)), ao3 = V_AO(NB(1, 0, 2), NB(2, 0, 1), NB(2, 0, 2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1, -1), 1, data->m_bottom_tex_layer, ao0));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1, -1), 0, data->m_bottom_tex_layer, ao1));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1,  1), 3, data->m_bottom_tex_layer, ao2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1,  1), 2, data->m_bottom_tex_layer, ao3));
                        push_face_indices(ao0 + ao3 > ao1 + ao2);
                    }
                    if (north_visible) {
                        u8 ao0 = V_AO(NB(2, 1, 0), NB(2, 0, 1), NB(2, 0, 0)), ao1 = V_AO(NB(2, 1, 0), NB(2, 2, 1), NB(2, 2, 0));
                        u8 ao2 = V_AO(NB(2, 1, 2), NB(2, 0, 1), NB(2, 0, 2)), ao3 = V_AO(NB(2, 1, 2), NB(2, 2, 1), NB(2, 2, 2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1, -1), 1, data->m_north_tex_layer, ao0));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1, -1), 0, data->m_north_tex_layer, ao1));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1,  1), 3, data->m_north_tex_layer, ao2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1,  1), 2, data->m_north_tex_layer, ao3));
                        push_face_indices(ao0 + ao3 > ao1 + ao2);
                    }
                    if (south_visible) {
                        u8 ao0 = V_AO(NB(0, 1, 0), NB(0, 0, 1), NB(0, 0, 0)), ao1 = V_AO(NB(0, 1, 2), NB(0, 0, 1), NB(0, 0, 2));
                        u8 ao2 = V_AO(NB(0, 1, 0), NB(0, 2, 1), NB(0, 2, 0)), ao3 = V_AO(NB(0, 1, 2), NB(0, 2, 1), NB(0, 2, 2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1, -1), 1, data->m_south_tex_layer, ao0));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1,  1), 3, data->m_south_tex_layer, ao1));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1, -1), 0, data->m_south_tex_layer, ao2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1,  1), 2, data->m_south_tex_layer, ao3));
                        push_face_indices(ao0 + ao3 > ao1 + ao2);
                    }
                    if (west_visible) {
                        u8 ao0 = V_AO(NB(0, 1, 0), NB(1, 0, 0), NB(0, 0, 0)), ao1 = V_AO(NB(0, 1, 0), NB(1, 2, 0), NB(0, 2, 0));
                        u8 ao2 = V_AO(NB(2, 1, 0), NB(1, 0, 0), NB(2, 0, 0)), ao3 = V_AO(NB(1, 2, 0), NB(2, 1, 0), NB(2, 2, 0));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1, -1), 1, data->m_west_tex_layer, ao0));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1, -1), 0, data->m_west_tex_layer, ao1));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1, -1), 3, data->m_west_tex_layer, ao2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1, -1), 2, data->m_west_tex_layer, ao3));
                        push_face_indices(ao0 + ao3 > ao1 + ao2);
                    }
                    if (east_visible) {
                        u8 ao0 = V_AO(NB(0, 1, 2), NB(1, 0, 2), NB(0, 0, 2)), ao1 = V_AO(NB(2, 1, 2), NB(1, 0, 2), NB(2, 0, 2));
                        u8 ao2 = V_AO(NB(0, 1, 2), NB(1, 2, 2), NB(0, 2, 2)), ao3 = V_AO(NB(2, 1, 2), NB(1, 2, 2), NB(2, 2, 2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1,  1), 1, data->m_east_tex_layer, ao0));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1,  1), 3, data->m_east_tex_layer, ao1));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1,  1), 0, data->m_east_tex_layer, ao2));
                        m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1,  1), 2, data->m_east_tex_layer, ao3));
                        push_face_indices(ao0 + ao3 > ao1 + ao2);
                    }

                    current_block++;
                }
            }
        }
    }
#undef V_AO
#undef NB
#undef NC
}
