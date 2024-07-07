#include "chunk_mesh_builder.hpp"
#include "chunk_renderer.hpp"
#include "../game.hpp"
#include "src/block/block.hpp"
#include "src/world/chunk.hpp"

namespace renderer {
    ChunkMeshBuilder::ChunkMeshBuilder() {}
    ChunkMeshBuilder::~ChunkMeshBuilder() {}

    void ChunkMeshBuilder::push_transparent_face_indices() {
        m_transparent_indices.push_back(m_last_transparent_index + 1);
        m_transparent_indices.push_back(m_last_transparent_index + 3);
        m_transparent_indices.push_back(m_last_transparent_index + 2);
        m_transparent_indices.push_back(m_last_transparent_index + 2);
        m_transparent_indices.push_back(m_last_transparent_index + 3);
        m_transparent_indices.push_back(m_last_transparent_index + 4);
        m_last_transparent_index += 4;
    }

std::mutex pritn_mutex;
#define NC(x, y, z) neighbor_chunks[coords_to_index<3>(x, y, z)]
#define NB(x, y, z) neighbor_blocks[coords_to_index<3>(x, y, z)]
#define NL(x, y, z) neighbor_light[coords_to_index<3>(x, y, z)]
    bool ChunkMeshBuilder::build(Chunk *chunk) {
        auto world = Game::get()->world();
        m_chunk = chunk;

        Chunk *neighbor_chunks[3 * 3 * 3];
        bool missing_chunk = false;
        for (i32 y = -1; y < 2; y++) {
            for (i32 z = -1; z < 2; z++) {
                for (i32 x = -1; x < 2; x++) {
                    auto current_chunk = world->get_or_queue_chunk(chunk->m_chunk_pos + i32vec3(x, y, z));
                    if (!current_chunk || !current_chunk->m_decorated.test())
                        missing_chunk = true;
                    NC(x + 1, y + 1, z + 1) = current_chunk;
                }
            }
        }

        if (missing_chunk)
            return false;

        BlockNID neighbor_blocks[3 * 3 * 3];
        u16 neighbor_light[3 * 3 * 3];

        m_transparent_vertices.clear();
        m_transparent_indices.clear();
        m_last_transparent_index = -1;
        m_faces.clear();

        if (chunk->m_storage.m_bits_per_index == 0 && chunk->m_storage.m_data.single->type == 0)
            return true; // the chunk is just void

        // FIXME: handle the case where the single type isnt void
        u32 unpacked[Chunk::volume];
        PackedArray_unpack(chunk->m_storage.m_data.packed, 0, unpacked, Chunk::volume);
        for (usize i = 0; i < Chunk::volume; i++)
            unpacked[i] = chunk->m_storage.m_palette[unpacked[i]].type;

        for (uint y = 0; y < Chunk::size; y++) {
            for (uint z = 0; z < Chunk::size; z++) {
                for (uint x = 0; x < Chunk::size; x++) {
                    BlockNID current_block = unpacked[coords_to_index<Chunk::size>(x, y, z)];
                    if (current_block == 0) // guaranteed to be void
                        continue;

                    BlockData *data = get_block_data(current_block);

                    for (i32 nx = -1; nx < 2; nx++) {
                        for (i32 ny = -1; ny < 2; ny++) {
                            for (i32 nz = -1; nz < 2; nz++) {
                                i32 cx = 1, cy = 1, cz = 1, rx = x + nx, ry = y + ny, rz = z + nz;
                                if (rx == -1) { cx = 0; rx = Chunk::size - 1; }
                                else if (rx == Chunk::size) { cx = 2; rx = 0; }
                                if (ry == -1) { cy = 0; ry = Chunk::size - 1; }
                                else if (ry == Chunk::size) { cy = 2; ry = 0; }
                                if (rz == -1) { cz = 0; rz = Chunk::size - 1; } 
                                else if (rz == Chunk::size) { cz = 2; rz = 0; }

                                if (cx == 1 && cy == 1 && cz == 1) { // current chunk
                                    NB(nx + 1, ny + 1, nz + 1) = unpacked[coords_to_index<Chunk::size>(rx, ry, rz)];
                                    NL(nx + 1, ny + 1, nz + 1) = chunk->m_light_map[coords_to_index<Chunk::size>(rx, ry, rz)];
                                    continue;
                                }

                                auto c = NC(cx, cy, cz);
                                NB(nx + 1, ny + 1, nz + 1) = c->m_storage.get_block(coords_to_index<Chunk::size>(rx, ry, rz));
                                NL(nx + 1, ny + 1, nz + 1) = c->m_light_map[coords_to_index<Chunk::size>(rx, ry, rz)];
                            }
                        }
                    }

                    bool top_visible = NB(1, 2, 1) == 0 || (!data->m_top_transparent && get_block_data(NB(1, 2, 1))->m_bottom_transparent);
                    bool bottom_visible = NB(1, 0, 1) == 0 || (!data->m_bottom_transparent && get_block_data(NB(1, 0, 1))->m_top_transparent);
                    bool north_visible = NB(2, 1, 1) == 0 || (!data->m_north_transparent && get_block_data(NB(2, 1, 1))->m_south_transparent);
                    bool south_visible = NB(0, 1, 1) == 0 || (!data->m_south_transparent && get_block_data(NB(0, 1, 1))->m_north_transparent);
                    bool east_visible = NB(1, 1, 2) == 0 || (!data->m_east_transparent && get_block_data(NB(1, 1, 2))->m_west_transparent);
                    bool west_visible = NB(1, 1, 0) == 0 || (!data->m_west_transparent && get_block_data(NB(1, 1, 0))->m_east_transparent);

                    if (!data->m_top_transparent) { // solid blocks
                        if (top_visible) {
                            auto &face = m_faces.emplace_back(x, y, z, 0, data->m_top_tex_index);
                            face.set_vertex_data(0, NL(0, 2, 1), NL(1, 2, 0), NL(0, 2, 0), NL(1, 2, 1)); // south west
                            face.set_vertex_data(1, NL(0, 2, 1), NL(1, 2, 2), NL(0, 2, 2), NL(1, 2, 1)); // south east
                            face.set_vertex_data(2, NL(2, 2, 1), NL(1, 2, 0), NL(2, 2, 0), NL(1, 2, 1)); // north west
                            face.set_vertex_data(3, NL(2, 2, 1), NL(1, 2, 2), NL(2, 2, 2), NL(1, 2, 1)); // north east
                        }
                        if (bottom_visible) {
                            auto &face = m_faces.emplace_back(x, y, z, 1, data->m_bottom_tex_index);
                            face.set_vertex_data(0, NL(0, 0, 1), NL(1, 0, 0), NL(0, 0, 0), NL(1, 0, 1)); // south west
                            face.set_vertex_data(1, NL(2, 0, 1), NL(1, 0, 0), NL(2, 0, 0), NL(1, 0, 1)); // north west
                            face.set_vertex_data(2, NL(0, 0, 1), NL(1, 0, 2), NL(0, 0, 2), NL(1, 0, 1)); // south east
                            face.set_vertex_data(3, NL(2, 0, 1), NL(1, 0, 2), NL(2, 0, 2), NL(1, 0, 1)); // north east
                        }
                        if (north_visible) {
                            auto &face = m_faces.emplace_back(x, y, z, 2, data->m_north_tex_index);
                            face.set_vertex_data(0, NL(2, 0, 1), NL(2, 1, 0), NL(2, 0, 0), NL(2, 1, 1)); // bottom west
                            face.set_vertex_data(1, NL(2, 2, 1), NL(2, 1, 0), NL(2, 2, 0), NL(2, 1, 1)); // top west
                            face.set_vertex_data(2, NL(2, 0, 1), NL(2, 1, 2), NL(2, 0, 2), NL(2, 1, 1)); // bottom east
                            face.set_vertex_data(3, NL(2, 2, 1), NL(2, 1, 2), NL(2, 2, 2), NL(2, 1, 1)); // top east
                        }
                        if (south_visible) {
                            auto &face = m_faces.emplace_back(x, y, z, 3, data->m_south_tex_index);
                            face.set_vertex_data(0, NL(0, 0, 1), NL(0, 1, 0), NL(0, 0, 0), NL(0, 1, 1)); // bottom west
                            face.set_vertex_data(1, NL(0, 0, 1), NL(0, 1, 2), NL(0, 0, 2), NL(0, 1, 1)); // bottom east
                            face.set_vertex_data(2, NL(0, 2, 1), NL(0, 1, 0), NL(0, 2, 0), NL(0, 1, 1)); // top west
                            face.set_vertex_data(3, NL(0, 2, 1), NL(0, 1, 2), NL(0, 2, 2), NL(0, 1, 1)); // top east
                        }
                        if (east_visible) {
                            auto &face = m_faces.emplace_back(x, y, z, 4, data->m_east_tex_index);
                            face.set_vertex_data(0, NL(0, 1, 2), NL(1, 0, 2), NL(0, 0, 2), NL(1, 1, 2)); // bottom south
                            face.set_vertex_data(1, NL(2, 1, 2), NL(1, 0, 2), NL(2, 0, 2), NL(1, 1, 2)); // bottom north
                            face.set_vertex_data(2, NL(0, 1, 2), NL(1, 2, 2), NL(0, 2, 2), NL(1, 1, 2)); // top south
                            face.set_vertex_data(3, NL(2, 1, 2), NL(1, 2, 2), NL(2, 2, 2), NL(1, 1, 2)); // top north
                        }
                        if (west_visible) {
                            auto &face = m_faces.emplace_back(x, y, z, 5, data->m_west_tex_index);
                            face.set_vertex_data(0, NL(0, 1, 0), NL(1, 0, 0), NL(0, 0, 0), NL(1, 1, 0)); // bottom south
                            face.set_vertex_data(1, NL(0, 1, 0), NL(1, 2, 0), NL(0, 2, 0), NL(1, 1, 0)); // top south
                            face.set_vertex_data(2, NL(2, 1, 0), NL(1, 0, 0), NL(2, 0, 0), NL(1, 1, 0)); // bottom north
                            face.set_vertex_data(3, NL(2, 1, 0), NL(1, 2, 0), NL(2, 2, 0), NL(1, 1, 0)); // top north
                        }
                    } else { // transparent blocks
                        auto pos = vec3(x * 2, y * 2, z * 2);
                        switch (data->m_shape) {
                        case BlockShape::FULL_BLOCK:
                            if (top_visible) {
                                m_transparent_vertices.emplace_back(pos + vec3(-1,  1, -1), 1, data->m_top_tex_index, NL(0, 2, 1), NL(1, 2, 0), NL(0, 2, 0), NL(1, 2, 1));
                                m_transparent_vertices.emplace_back(pos + vec3(-1,  1,  1), 3, data->m_top_tex_index, NL(0, 2, 1), NL(1, 2, 2), NL(0, 2, 2), NL(1, 2, 1));
                                m_transparent_vertices.emplace_back(pos + vec3( 1,  1, -1), 0, data->m_top_tex_index, NL(2, 2, 1), NL(1, 2, 0), NL(2, 2, 0), NL(1, 2, 1));
                                m_transparent_vertices.emplace_back(pos + vec3( 1,  1,  1), 2, data->m_top_tex_index, NL(2, 2, 1), NL(1, 2, 2), NL(2, 2, 2), NL(1, 2, 1));
                                push_transparent_face_indices();
                            }
                            if (bottom_visible) {
                                m_transparent_vertices.emplace_back(pos + vec3(-1, -1, -1), 1, data->m_bottom_tex_index, NL(0, 0, 1), NL(1, 0, 0), NL(0, 0, 0), NL(1, 0, 1));
                                m_transparent_vertices.emplace_back(pos + vec3( 1, -1, -1), 0, data->m_bottom_tex_index, NL(2, 0, 1), NL(1, 0, 0), NL(2, 0, 0), NL(1, 0, 1));
                                m_transparent_vertices.emplace_back(pos + vec3(-1, -1,  1), 3, data->m_bottom_tex_index, NL(0, 0, 1), NL(1, 0, 2), NL(0, 0, 2), NL(1, 0, 1));
                                m_transparent_vertices.emplace_back(pos + vec3( 1, -1,  1), 2, data->m_bottom_tex_index, NL(2, 0, 1), NL(1, 0, 2), NL(2, 0, 2), NL(1, 0, 1));
                                push_transparent_face_indices();
                            }
                            if (north_visible) {
                                m_transparent_vertices.emplace_back(pos + vec3( 1, -1, -1), 1, data->m_north_tex_index, NL(2, 0, 1), NL(2, 1, 0), NL(2, 0, 0), NL(2, 1, 1));
                                m_transparent_vertices.emplace_back(pos + vec3( 1,  1, -1), 0, data->m_north_tex_index, NL(2, 2, 1), NL(2, 1, 0), NL(2, 2, 0), NL(2, 1, 1));
                                m_transparent_vertices.emplace_back(pos + vec3( 1, -1,  1), 3, data->m_north_tex_index, NL(2, 0, 1), NL(2, 1, 2), NL(2, 0, 2), NL(2, 1, 1));
                                m_transparent_vertices.emplace_back(pos + vec3( 1,  1,  1), 2, data->m_north_tex_index, NL(2, 2, 1), NL(2, 1, 2), NL(2, 2, 2), NL(2, 1, 1));
                                push_transparent_face_indices();
                            }
                            if (south_visible) {
                                m_transparent_vertices.emplace_back(pos + vec3(-1, -1, -1), 1, data->m_south_tex_index, NL(0, 0, 1), NL(0, 1, 0), NL(0, 0, 0), NL(0, 1, 1));
                                m_transparent_vertices.emplace_back(pos + vec3(-1, -1,  1), 3, data->m_south_tex_index, NL(0, 0, 1), NL(0, 1, 2), NL(0, 0, 2), NL(0, 1, 1));
                                m_transparent_vertices.emplace_back(pos + vec3(-1,  1, -1), 0, data->m_south_tex_index, NL(0, 2, 1), NL(0, 1, 0), NL(0, 2, 0), NL(0, 1, 1));
                                m_transparent_vertices.emplace_back(pos + vec3(-1,  1,  1), 2, data->m_south_tex_index, NL(0, 2, 1), NL(0, 1, 2), NL(0, 2, 2), NL(0, 1, 1));
                                push_transparent_face_indices();
                            }
                            if (east_visible) {
                                m_transparent_vertices.emplace_back(pos + vec3(-1, -1,  1), 1, data->m_east_tex_index, NL(0, 1, 2), NL(1, 0, 2), NL(0, 0, 2), NL(1, 1, 2));
                                m_transparent_vertices.emplace_back(pos + vec3( 1, -1,  1), 3, data->m_east_tex_index, NL(2, 1, 2), NL(1, 0, 2), NL(2, 0, 2), NL(1, 1, 2));
                                m_transparent_vertices.emplace_back(pos + vec3(-1,  1,  1), 0, data->m_east_tex_index, NL(0, 1, 2), NL(1, 2, 2), NL(0, 2, 2), NL(1, 1, 2));
                                m_transparent_vertices.emplace_back(pos + vec3( 1,  1,  1), 2, data->m_east_tex_index, NL(2, 1, 2), NL(1, 2, 2), NL(2, 2, 2), NL(1, 1, 2));
                                push_transparent_face_indices();
                            }
                            if (west_visible) {
                                m_transparent_vertices.emplace_back(pos + vec3(-1, -1, -1), 1, data->m_west_tex_index, NL(0, 1, 0), NL(1, 0, 0), NL(0, 0, 0), NL(1, 1, 0));
                                m_transparent_vertices.emplace_back(pos + vec3(-1,  1, -1), 0, data->m_west_tex_index, NL(0, 1, 0), NL(1, 2, 0), NL(0, 2, 0), NL(1, 1, 0));
                                m_transparent_vertices.emplace_back(pos + vec3( 1, -1, -1), 3, data->m_west_tex_index, NL(2, 1, 0), NL(1, 0, 0), NL(2, 0, 0), NL(1, 1, 0));
                                m_transparent_vertices.emplace_back(pos + vec3( 1,  1, -1), 2, data->m_west_tex_index, NL(2, 1, 0), NL(1, 2, 0), NL(2, 2, 0), NL(1, 1, 0));
                                push_transparent_face_indices();
                            }
                            break;
                        case BlockShape::X_SHAPE:
                            if (top_visible || bottom_visible || north_visible || south_visible || east_visible || west_visible) {
                                u16 sl = NL(1, 1, 1);
                                m_transparent_vertices.emplace_back(pos + vec3(-0.7, -1, -0.7), 1, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3(-0.7,  1, -0.7), 0, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3( 0.7, -1,  0.7), 3, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3( 0.7,  1,  0.7), 2, data->m_top_tex_index, sl, sl, sl, sl);
                                push_transparent_face_indices();

                                m_transparent_vertices.emplace_back(pos + vec3(-0.7, -1, -0.7), 1, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3( 0.7, -1,  0.7), 3, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3(-0.7,  1, -0.7), 0, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3( 0.7,  1,  0.7), 2, data->m_top_tex_index, sl, sl, sl, sl);
                                push_transparent_face_indices();

                                m_transparent_vertices.emplace_back(pos + vec3( 0.7, -1, -0.7), 1, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3( 0.7,  1, -0.7), 0, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3(-0.7, -1,  0.7), 3, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3(-0.7,  1,  0.7), 2, data->m_top_tex_index, sl, sl, sl, sl);
                                push_transparent_face_indices();

                                m_transparent_vertices.emplace_back(pos + vec3( 0.7, -1, -0.7), 1, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3(-0.7, -1,  0.7), 3, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3( 0.7,  1, -0.7), 0, data->m_top_tex_index, sl, sl, sl, sl);
                                m_transparent_vertices.emplace_back(pos + vec3(-0.7,  1,  0.7), 2, data->m_top_tex_index, sl, sl, sl, sl);
                                push_transparent_face_indices();
                            }
                        }
                    }

                    current_block++;
                }
            }
        }
        return true;
    }
#undef NL
#undef NB
#undef NC
}
