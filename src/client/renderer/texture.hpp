#pragma once

#include "context.hpp"

namespace render {
    class Texture {
        Context &m_context;

        void create_image(VmaAllocationInfo *alloc_info, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
        void transition_image_layout(VkImageLayout old_layout, VkImageLayout new_layout);
        void copy_buffer_to_image(VkBuffer buffer);

    public:
        usize m_texture_width;
        usize m_texture_height;
        usize m_texture_layers;

        VkImage m_image;
        VmaAllocation m_image_allocation;
        VkImageView m_image_view;
        VkSampler m_sampler;

        Texture(Context &context) : m_context(context) {}
        void init();
        void cleanup();
    };
}
