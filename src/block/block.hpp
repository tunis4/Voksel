#pragma once

#include <string>
#include <vector>

#include "../util.hpp"

enum class BlockFace {
    TOP, BOTTOM, NORTH, SOUTH, EAST, WEST
};

enum class BlockShape {
    FULL_BLOCK, X_SHAPE
};

using BlockNID = u32; // block numerical id
struct BlockData {
    std::string m_id;

    BlockShape m_shape;

    // texture descriptor indices
    u32 m_top_tex_index, m_bottom_tex_index, m_north_tex_index, m_south_tex_index, m_east_tex_index, m_west_tex_index;
    inline void set_textures(u32 all_faces) { set_textures(all_faces, all_faces, all_faces, all_faces, all_faces, all_faces); }
    inline void set_textures(u32 top_face, u32 bottom_face, u32 side_faces) { set_textures(top_face, bottom_face, side_faces, side_faces, side_faces, side_faces); }
    void set_textures(u32 top_face, u32 bottom_face, u32 north_face, u32 south_face, u32 east_face, u32 west_face);

    // FIXME: the code currently doesnt care about the individual faces, only the top face transparency is checked
    bool m_top_transparent, m_bottom_transparent, m_north_transparent, m_south_transparent, m_east_transparent, m_west_transparent;
    inline void set_transparency(bool all_faces) { set_transparency(all_faces, all_faces, all_faces, all_faces, all_faces, all_faces); }
    void set_transparency(bool top_face, bool bottom_face, bool north_face, bool south_face, bool east_face, bool west_face);

    bool m_collidable;

    BlockData(std::string id);
};

BlockData* get_block_data(BlockNID nid);
usize num_block_data();
void register_blocks();
