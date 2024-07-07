#include "block.hpp"
#include "../renderer/texture.hpp"

#include <entt/entt.hpp>
using namespace entt::literals;

BlockData::BlockData(std::string id) {
    m_id = id;

    // set default values
    m_shape = BlockShape::FULL_BLOCK;
    set_transparency(false);
    m_collidable = true;
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
BlockData* get_block_data(BlockNID nid) {
    return &block_data_registry[nid];
}

usize num_block_data() { return block_data_registry.size(); }

static u32 load_block_texture(const std::string &name) {
    return entt::locator<renderer::TextureManager>::value().load_texture(name);
}

void register_blocks() {
    BlockData void_block("void");
    void_block.set_textures(0);
    void_block.set_transparency(true);
    void_block.m_collidable = false;
    block_data_registry.push_back(void_block);

    BlockData stone("stone");
    u32 stone_texture = load_block_texture("stone");
    stone.set_textures(stone_texture);
    block_data_registry.push_back(stone);

    BlockData cobblestone("cobblestone");
    u32 cobblestone_texture = load_block_texture("cobblestone");
    cobblestone.set_textures(cobblestone_texture);
    block_data_registry.push_back(cobblestone);

    BlockData dirt("dirt");
    u32 dirt_texture = load_block_texture("dirt");
    dirt.set_textures(dirt_texture);
    block_data_registry.push_back(dirt);

    BlockData grass_block("grass_block");
    u32 grass_block_top_texture = load_block_texture("grass_block_top");
    u32 grass_block_side_texture = load_block_texture("grass_block_side");
    grass_block.set_textures(grass_block_top_texture, dirt_texture, grass_block_side_texture);
    block_data_registry.push_back(grass_block);

    BlockData planks("planks");
    u32 planks_texture = load_block_texture("planks");
    planks.set_textures(planks_texture);
    block_data_registry.push_back(planks);

    BlockData wood_log("wood_log");
    u32 wood_log_top_texture = load_block_texture("wood_log_top");
    u32 wood_log_side_texture = load_block_texture("wood_log_side");
    wood_log.set_textures(wood_log_top_texture, wood_log_top_texture, wood_log_side_texture);
    block_data_registry.push_back(wood_log);

    BlockData sand("sand");
    u32 sand_texture = load_block_texture("sand");
    sand.set_textures(sand_texture);
    block_data_registry.push_back(sand);

    BlockData glass("glass");
    u32 glass_texture = load_block_texture("glass");
    glass.set_textures(glass_texture);
    glass.set_transparency(true);
    block_data_registry.push_back(glass);

    BlockData bricks("bricks");
    u32 bricks_texture = load_block_texture("bricks");
    bricks.set_textures(bricks_texture);
    block_data_registry.push_back(bricks);

    BlockData water("water");
    u32 water_texture = load_block_texture("water");
    water.set_textures(water_texture);
    water.set_transparency(true);
    water.m_collidable = false;
    block_data_registry.push_back(water);

    BlockData grass("grass");
    u32 grass_texture = load_block_texture("grass");
    grass.set_textures(grass_texture);
    grass.set_transparency(true);
    grass.m_shape = BlockShape::X_SHAPE;
    grass.m_collidable = false;
    block_data_registry.push_back(grass);

    BlockData leaves("leaves");
    u32 leaves_texture = load_block_texture("leaves");
    leaves.set_textures(leaves_texture);
    block_data_registry.push_back(leaves);

    BlockData dandelion("dandelion");
    u32 dandelion_texture = load_block_texture("dandelion");
    dandelion.set_textures(dandelion_texture);
    dandelion.set_transparency(true);
    dandelion.m_shape = BlockShape::X_SHAPE;
    dandelion.m_collidable = false;
    block_data_registry.push_back(dandelion);

    BlockData rose("rose");
    u32 rose_texture = load_block_texture("rose");
    rose.set_textures(rose_texture);
    rose.set_transparency(true);
    rose.m_shape = BlockShape::X_SHAPE;
    rose.m_collidable = false;
    block_data_registry.push_back(rose);
}
