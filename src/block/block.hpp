#pragma once

#include <string>

#include "../util/util.hpp"

namespace block {
    enum class Face {
        TOP, BOTTOM, NORTH, SOUTH, EAST, WEST
    };

    using NID = u32; // block numerical id
    struct BlockData {
        std::string m_id;

        // texture layers
        u32 m_top_tex_layer, m_bottom_tex_layer, m_north_tex_layer, m_south_tex_layer, m_east_tex_layer, m_west_tex_layer;

        BlockData(std::string id);
        
        inline void set_textures(u32 all_faces) { set_textures(all_faces, all_faces, all_faces, all_faces, all_faces, all_faces); }
        inline void set_textures(u32 top_face, u32 bottom_face, u32 side_faces) { set_textures(top_face, bottom_face, side_faces, side_faces, side_faces, side_faces); }
        void set_textures(u32 top_face, u32 bottom_face, u32 north_face, u32 south_face, u32 east_face, u32 west_face);
    };

    BlockData* get_block_data(NID nid);
    void register_blocks();
}
