#pragma once

#include "context.hpp"

namespace render {
    class Texture {
        Context &m_context;

        void create_image(VmaAllocationInfo *alloc_info, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
        void transition_image_layout(VkImageLayout old_layout, VkImageLayout new_layout);
        void copy_buffer_to_image(VkBuffer buffer);
        void generate_mipmaps();

    public:
        usize m_width;
        usize m_height;
        usize m_mip_levels;

        VkImage m_image;
        VmaAllocation m_image_allocation;
        VkImageView m_image_view;
        VkSampler m_sampler;

        Texture(Context &context) : m_context(context) {}
        void create(std::string_view filename);
        void destroy();
    };

    class TextureManager {
        Context &m_context;
        std::unordered_map<std::string, u32> m_texture_names_to_indices;
        
    public:
        std::vector<Texture> m_textures;
        
        TextureManager(Context &context) : m_context(context) {}
        void init();
        void cleanup();

        // returns the assigned descriptor index
        u32 load_texture(const std::string &name);
        u32 name_to_index(const std::string &name);
        u32 num_textures();
    };
}
