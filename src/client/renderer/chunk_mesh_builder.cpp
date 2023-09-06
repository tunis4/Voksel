#include "chunk_mesh_builder.hpp"
#include "../client.hpp"
#include "src/block/block.hpp"
#include "src/world/chunk.hpp"

namespace render {
    ChunkMeshBuilder::ChunkMeshBuilder(ChunkRenderer *renderer) {
        m_renderer = renderer;
    }

    ChunkMeshBuilder::~ChunkMeshBuilder() {}

    void ChunkMeshBuilder::push_face_indices(bool flipped) {
        // if (flipped) {
        //     m_indices.push_back(m_last_index + 1);
        //     m_indices.push_back(m_last_index + 4);
        //     m_indices.push_back(m_last_index + 2);
        //     m_indices.push_back(m_last_index + 1);
        //     m_indices.push_back(m_last_index + 3);
        //     m_indices.push_back(m_last_index + 4);
        // } else {
            m_indices.push_back(m_last_index + 1);
            m_indices.push_back(m_last_index + 3);
            m_indices.push_back(m_last_index + 2);
            m_indices.push_back(m_last_index + 2);
            m_indices.push_back(m_last_index + 3);
            m_indices.push_back(m_last_index + 4);
        // }
        m_last_index += 4;
    }

#define NC(x, y, z) neighbor_chunks[util::coords_to_index<3>(x, y, z)]
#define NB(x, y, z) neighbor_blocks[util::coords_to_index<3>(x, y, z)]
#define NL(x, y, z) neighbor_light[util::coords_to_index<3>(x, y, z)]
#define V_AO(side1, side2, corner) ((bool)side1 && (bool)side2) ? 0 : 3 - ((bool)side1 + (bool)side2 + (bool)corner)
    void ChunkMeshBuilder::build() {
        world::Chunk *neighbor_chunks[3 * 3 * 3];
        for (i32 y = -1; y < 2; y++)
            for (i32 z = -1; z < 2; z++)
                for (i32 x = -1; x < 2; x++)
                    NC(x + 1, y + 1, z + 1) = client::Client::get()->world()->get_or_generate_chunk((*m_current).m_chunk_pos + glm::i32vec3(x, y, z)).get();

        block::NID neighbor_blocks[3 * 3 * 3];
        u16 neighbor_light[3 * 3 * 3];

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

                                if (cx == 1 && cy == 1 && cz == 1) { // current chunk
                                    NB(nx + 1, ny + 1, nz + 1) = unpacked[util::coords_to_index<world::Chunk::size>(rx, ry, rz)];
                                    NL(nx + 1, ny + 1, nz + 1) = chunk->m_light_map[util::coords_to_index<world::Chunk::size>(rx, ry, rz)];
                                    continue;
                                }
                                
                                auto c = NC(cx, cy, cz);
                                NB(nx + 1, ny + 1, nz + 1) = c->m_storage.get_block(util::coords_to_index<world::Chunk::size>(rx, ry, rz));
                                NL(nx + 1, ny + 1, nz + 1) = c->m_light_map[util::coords_to_index<world::Chunk::size>(rx, ry, rz)];
                            }
                        }
                    }

                    bool top_visible = NB(1, 2, 1) == 0 || (!data->m_top_transparent && block::get_block_data(NB(1, 2, 1))->m_bottom_transparent);
                    bool bottom_visible = NB(1, 0, 1) == 0 || (!data->m_bottom_transparent && block::get_block_data(NB(1, 0, 1))->m_top_transparent);
                    bool north_visible = NB(2, 1, 1) == 0 || (!data->m_north_transparent && block::get_block_data(NB(2, 1, 1))->m_south_transparent);
                    bool south_visible = NB(0, 1, 1) == 0 || (!data->m_south_transparent && block::get_block_data(NB(0, 1, 1))->m_north_transparent);
                    bool east_visible = NB(1, 1, 2) == 0 || (!data->m_east_transparent && block::get_block_data(NB(1, 1, 2))->m_west_transparent);
                    bool west_visible = NB(1, 1, 0) == 0 || (!data->m_west_transparent && block::get_block_data(NB(1, 1, 0))->m_east_transparent);
                
                    auto pos = glm::vec3(x * 2, y * 2, z * 2);
                    switch (data->m_shape) {
                    case block::Shape::FULL_BLOCK:
                        if (top_visible) {
                            u8 ao0 = V_AO(NB(0, 2, 1), NB(1, 2, 0), NB(0, 2, 0)), ao1 = V_AO(NB(0, 2, 1), NB(1, 2, 2), NB(0, 2, 2));
                            u8 ao2 = V_AO(NB(1, 2, 0), NB(2, 2, 1), NB(2, 2, 0)), ao3 = V_AO(NB(1, 2, 2), NB(2, 2, 1), NB(2, 2, 2));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1, -1), 1, data->m_top_tex_index, NL(0, 2, 1), NL(1, 2, 0), NL(0, 2, 0), NL(1, 2, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1,  1), 3, data->m_top_tex_index, NL(0, 2, 1), NL(1, 2, 2), NL(0, 2, 2), NL(1, 2, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1, -1), 0, data->m_top_tex_index, NL(2, 2, 1), NL(1, 2, 0), NL(2, 2, 0), NL(1, 2, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1,  1), 2, data->m_top_tex_index, NL(2, 2, 1), NL(1, 2, 2), NL(2, 2, 2), NL(1, 2, 1)));
                            push_face_indices(ao0 + ao3 > ao1 + ao2);
                        }
                        if (bottom_visible) {
                            u8 ao0 = V_AO(NB(0, 0, 1), NB(1, 0, 0), NB(0, 0, 0)), ao1 = V_AO(NB(2, 0, 1), NB(1, 0, 0), NB(2, 0, 0));
                            u8 ao2 = V_AO(NB(0, 0, 1), NB(1, 0, 2), NB(0, 0, 2)), ao3 = V_AO(NB(1, 0, 2), NB(2, 0, 1), NB(2, 0, 2));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1, -1), 1, data->m_bottom_tex_index, NL(0, 0, 1), NL(1, 0, 0), NL(0, 0, 0), NL(1, 0, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1, -1), 0, data->m_bottom_tex_index, NL(2, 0, 1), NL(1, 0, 0), NL(2, 0, 0), NL(1, 0, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1,  1), 3, data->m_bottom_tex_index, NL(0, 0, 1), NL(1, 0, 2), NL(0, 0, 2), NL(1, 0, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1,  1), 2, data->m_bottom_tex_index, NL(2, 0, 1), NL(1, 0, 2), NL(2, 0, 2), NL(1, 0, 1)));
                            push_face_indices(ao0 + ao3 > ao1 + ao2);
                        }
                        if (north_visible) {
                            u8 ao0 = V_AO(NB(2, 1, 0), NB(2, 0, 1), NB(2, 0, 0)), ao1 = V_AO(NB(2, 1, 0), NB(2, 2, 1), NB(2, 2, 0));
                            u8 ao2 = V_AO(NB(2, 1, 2), NB(2, 0, 1), NB(2, 0, 2)), ao3 = V_AO(NB(2, 1, 2), NB(2, 2, 1), NB(2, 2, 2));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1, -1), 1, data->m_north_tex_index, NL(2, 0, 1), NL(2, 1, 0), NL(2, 0, 0), NL(2, 1, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1, -1), 0, data->m_north_tex_index, NL(2, 2, 1), NL(2, 1, 0), NL(2, 2, 0), NL(2, 1, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1,  1), 3, data->m_north_tex_index, NL(2, 0, 1), NL(2, 1, 2), NL(2, 0, 2), NL(2, 1, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1,  1), 2, data->m_north_tex_index, NL(2, 2, 1), NL(2, 1, 2), NL(2, 2, 2), NL(2, 1, 1)));
                            push_face_indices(ao0 + ao3 > ao1 + ao2);
                        }
                        if (south_visible) {
                            u8 ao0 = V_AO(NB(0, 1, 0), NB(0, 0, 1), NB(0, 0, 0)), ao1 = V_AO(NB(0, 1, 2), NB(0, 0, 1), NB(0, 0, 2));
                            u8 ao2 = V_AO(NB(0, 1, 0), NB(0, 2, 1), NB(0, 2, 0)), ao3 = V_AO(NB(0, 1, 2), NB(0, 2, 1), NB(0, 2, 2));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1, -1), 1, data->m_south_tex_index, NL(0, 0, 1), NL(0, 1, 0), NL(0, 0, 0), NL(0, 1, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1,  1), 3, data->m_south_tex_index, NL(0, 0, 1), NL(0, 1, 2), NL(0, 0, 2), NL(0, 1, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1, -1), 0, data->m_south_tex_index, NL(0, 2, 1), NL(0, 1, 0), NL(0, 2, 0), NL(0, 1, 1)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1,  1), 2, data->m_south_tex_index, NL(0, 2, 1), NL(0, 1, 2), NL(0, 2, 2), NL(0, 1, 1)));
                            push_face_indices(ao0 + ao3 > ao1 + ao2);
                        }
                        if (east_visible) {
                            u8 ao0 = V_AO(NB(0, 1, 2), NB(1, 0, 2), NB(0, 0, 2)), ao1 = V_AO(NB(2, 1, 2), NB(1, 0, 2), NB(2, 0, 2));
                            u8 ao2 = V_AO(NB(0, 1, 2), NB(1, 2, 2), NB(0, 2, 2)), ao3 = V_AO(NB(2, 1, 2), NB(1, 2, 2), NB(2, 2, 2));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1,  1), 1, data->m_east_tex_index, NL(0, 1, 2), NL(1, 0, 2), NL(0, 0, 2), NL(1, 1, 2)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1,  1), 3, data->m_east_tex_index, NL(2, 1, 2), NL(1, 0, 2), NL(2, 0, 2), NL(1, 1, 2)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1,  1), 0, data->m_east_tex_index, NL(0, 1, 2), NL(1, 2, 2), NL(0, 2, 2), NL(1, 1, 2)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1,  1), 2, data->m_east_tex_index, NL(2, 1, 2), NL(1, 2, 2), NL(2, 2, 2), NL(1, 1, 2)));
                            push_face_indices(ao0 + ao3 > ao1 + ao2);
                        }
                        if (west_visible) {
                            u8 ao0 = V_AO(NB(0, 1, 0), NB(1, 0, 0), NB(0, 0, 0)), ao1 = V_AO(NB(0, 1, 0), NB(1, 2, 0), NB(0, 2, 0));
                            u8 ao2 = V_AO(NB(2, 1, 0), NB(1, 0, 0), NB(2, 0, 0)), ao3 = V_AO(NB(1, 2, 0), NB(2, 1, 0), NB(2, 2, 0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1, -1, -1), 1, data->m_west_tex_index, NL(0, 1, 0), NL(1, 0, 0), NL(0, 0, 0), NL(1, 1, 0)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-1,  1, -1), 0, data->m_west_tex_index, NL(0, 1, 0), NL(1, 2, 0), NL(0, 2, 0), NL(1, 1, 0)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1, -1, -1), 3, data->m_west_tex_index, NL(2, 1, 0), NL(1, 0, 0), NL(2, 0, 0), NL(1, 1, 0)));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 1,  1, -1), 2, data->m_west_tex_index, NL(2, 1, 0), NL(1, 2, 0), NL(2, 2, 0), NL(1, 1, 0)));
                            push_face_indices(ao0 + ao3 > ao1 + ao2);
                        }
                        break;
                    case block::Shape::X_SHAPE:
                        if (top_visible || bottom_visible || north_visible || south_visible || east_visible || west_visible) {
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-0.7, -1, -0.7), 1, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-0.7,  1, -0.7), 0, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 0.7, -1,  0.7), 3, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 0.7,  1,  0.7), 2, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            push_face_indices(false);
                            
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-0.7, -1, -0.7), 1, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 0.7, -1,  0.7), 3, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-0.7,  1, -0.7), 0, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 0.7,  1,  0.7), 2, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            push_face_indices(false);
                            
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 0.7, -1, -0.7), 1, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 0.7,  1, -0.7), 0, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-0.7, -1,  0.7), 3, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-0.7,  1,  0.7), 2, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            push_face_indices(false);
                            
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 0.7, -1, -0.7), 1, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-0.7, -1,  0.7), 3, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3( 0.7,  1, -0.7), 0, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            m_vertices.push_back(ChunkVertex(pos + glm::vec3(-0.7,  1,  0.7), 2, data->m_top_tex_index, ~0, ~0, ~0, ~0));
                            push_face_indices(false);
                        }
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
