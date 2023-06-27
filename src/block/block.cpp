#include "block.hpp"
#include "src/client/renderer/texture.hpp"

#include <array>
#include <entt/entt.hpp>
using namespace entt::literals;

namespace block {
    BlockData::BlockData(std::string id) {
        m_id = id;
        set_transparency(false);
    }

    void BlockData::set_textures(u32 top_face, u32 bottom_face, u32 north_face, 
        u32 south_face, u32 east_face, u32 west_face
    ) {
        m_top_tex_index = top_face; m_bottom_tex_index = bottom_face;
        m_north_tex_index = north_face; m_south_tex_index = south_face;
        m_east_tex_index = east_face; m_west_tex_index = west_face;
    }

    void BlockData::set_transparency(bool top_face, bool bottom_face, bool north_face, bool south_face, bool east_face, bool west_face) {
        m_top_transparent = top_face; m_bottom_transparent = bottom_face;
        m_north_transparent = north_face; m_south_transparent = south_face;
        m_east_transparent = east_face; m_west_transparent = west_face;
    }

    static std::vector<BlockData> block_data_registry;
    BlockData* get_block_data(NID nid) {
        return &block_data_registry[nid];
    }

    usize num_block_data() { return block_data_registry.size(); }

    void register_blocks() {
        render::TextureManager &texture_manager = entt::locator<render::TextureManager>::value();

        BlockData void_block("void");
        void_block.set_textures(0);
        block_data_registry.push_back(void_block);
        
        BlockData stone_block("stone");
        u32 stone_texture = texture_manager.load_texture("stone");
        stone_block.set_textures(stone_texture);
        block_data_registry.push_back(stone_block);
        
        BlockData cobblestone_block("cobblestone");
        u32 cobblestone_texture = texture_manager.load_texture("cobblestone");
        cobblestone_block.set_textures(cobblestone_texture);
        block_data_registry.push_back(cobblestone_block);
        
        BlockData dirt_block("dirt");
        u32 dirt_texture = texture_manager.load_texture("dirt");
        dirt_block.set_textures(dirt_texture);
        block_data_registry.push_back(dirt_block);
        
        BlockData grass_block("grass");
        u32 grass_top_texture = texture_manager.load_texture("grass_top");
        u32 grass_side_texture = texture_manager.load_texture("grass_side");
        grass_block.set_textures(grass_top_texture, dirt_texture, grass_side_texture);
        block_data_registry.push_back(grass_block);

        BlockData planks_block("planks");
        u32 planks_texture = texture_manager.load_texture("planks");
        planks_block.set_textures(planks_texture);
        block_data_registry.push_back(planks_block);
        
        BlockData wood_log_block("wood_log");
        u32 wood_log_top_texture = texture_manager.load_texture("wood_log_top");
        u32 wood_log_side_texture = texture_manager.load_texture("wood_log_side");
        wood_log_block.set_textures(wood_log_top_texture, wood_log_top_texture, wood_log_side_texture);
        block_data_registry.push_back(wood_log_block);
        
        BlockData sand_block("sand");
        u32 sand_texture = texture_manager.load_texture("sand");
        sand_block.set_textures(sand_texture);
        block_data_registry.push_back(sand_block);
        
        BlockData glass_block("glass");
        u32 glass_texture = texture_manager.load_texture("glass");
        glass_block.set_textures(glass_texture);
        glass_block.set_transparency(true);
        block_data_registry.push_back(glass_block);

        BlockData bricks_block("bricks");
        u32 bricks_texture = texture_manager.load_texture("bricks");
        bricks_block.set_textures(bricks_texture);
        block_data_registry.push_back(bricks_block);
        
        BlockData water_block("water");
        u32 water_texture = texture_manager.load_texture("water");
        water_block.set_textures(water_texture);
        water_block.set_transparency(true);
        block_data_registry.push_back(water_block);
    }
}
