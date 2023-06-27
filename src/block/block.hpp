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

        // texture descriptor indices
        u32 m_top_tex_index, m_bottom_tex_index, m_north_tex_index, m_south_tex_index, m_east_tex_index, m_west_tex_index;
        
        bool m_top_transparent, m_bottom_transparent, m_north_transparent, m_south_transparent, m_east_transparent, m_west_transparent;

        BlockData(std::string id);
        
        inline void set_textures(u32 all_faces) { set_textures(all_faces, all_faces, all_faces, all_faces, all_faces, all_faces); }
        inline void set_textures(u32 top_face, u32 bottom_face, u32 side_faces) { set_textures(top_face, bottom_face, side_faces, side_faces, side_faces, side_faces); }
        void set_textures(u32 top_face, u32 bottom_face, u32 north_face, u32 south_face, u32 east_face, u32 west_face);

        inline void set_transparency(bool all_faces) { set_transparency(all_faces, all_faces, all_faces, all_faces, all_faces, all_faces); }
        void set_transparency(bool top_face, bool bottom_face, bool north_face, bool south_face, bool east_face, bool west_face);
    };

    BlockData* get_block_data(NID nid);
    usize num_block_data();
    void register_blocks();
}
