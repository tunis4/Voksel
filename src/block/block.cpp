#include "block.hpp"
#include <array>

namespace block {
    BlockData::BlockData(std::string id) {
        m_id = id;
    }

    void BlockData::set_textures(u32 top_face, u32 bottom_face, u32 north_face, u32 south_face, u32 west_face, u32 east_face) {
        m_top_tex_layer = top_face; m_bottom_tex_layer = bottom_face;
        m_north_tex_layer = north_face; m_south_tex_layer = south_face;
        m_west_tex_layer = west_face; m_east_tex_layer = east_face;
    }

    static std::array<BlockData*, 9> block_data_registry;
    BlockData* get_block_data(NID nid) {
        return block_data_registry[nid];
    }

    void register_blocks() {
        BlockData *void_block = new BlockData("void");
        void_block->set_textures(0);
        block_data_registry.at(0) = void_block;
        
        BlockData *stone_block = new BlockData("stone");
        stone_block->set_textures(1);
        block_data_registry.at(1) = stone_block;
        
        BlockData *cobblestone_block = new BlockData("cobblestone");
        cobblestone_block->set_textures(2);
        block_data_registry.at(2) = cobblestone_block;
        
        BlockData *dirt_block = new BlockData("dirt");
        dirt_block->set_textures(3);
        block_data_registry.at(3) = dirt_block;
        
        BlockData *grass_block = new BlockData("grass");
        grass_block->set_textures(4, 3, 5);
        block_data_registry.at(4) = grass_block;

        BlockData *planks_block = new BlockData("planks");
        planks_block->set_textures(6);
        block_data_registry.at(5) = planks_block;
        
        BlockData *sand_block = new BlockData("sand");
        sand_block->set_textures(7);
        block_data_registry.at(6) = sand_block;
        
        BlockData *glass_block = new BlockData("glass");
        glass_block->set_textures(8);
        block_data_registry.at(7) = glass_block;
        
        BlockData *wood_log_block = new BlockData("wood_log");
        wood_log_block->set_textures(9, 9, 10);
        block_data_registry.at(8) = wood_log_block;
    }
}
