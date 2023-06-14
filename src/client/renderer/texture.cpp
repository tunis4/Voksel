#include "texture.hpp"

#include <cstring>
#include <stb/stb_image.h>

namespace render {
    static constexpr std::array texture_names = std::to_array<const char*>({
        "res/textures/error.png",
        "res/textures/stone.png",
        "res/textures/cobblestone.png",
        "res/textures/dirt.png",
        "res/textures/grass_top.png",
        "res/textures/grass_side.png",
        "res/textures/planks.png",
        "res/textures/sand.png",
        "res/textures/glass.png",
        "res/textures/wood_log_top.png",
        "res/textures/wood_log_side.png"
    });

    void Texture::init() {
        m_texture_width = 16;
        m_texture_height = 16;
        m_texture_layers = texture_names.size();

        VkDeviceSize image_size = m_texture_width * m_texture_height * m_texture_layers * 4;
        VkBuffer staging_buffer;
        VmaAllocation staging_buffer_allocation;

        m_context.create_buffer(&staging_buffer, &staging_buffer_allocation, nullptr, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        u8 *mapped_data;
        vmaMapMemory(m_context.allocator, staging_buffer_allocation, reinterpret_cast<void**>(&mapped_data));

        for (const char *texture_name : texture_names) {
            int width, height, channels;
            stbi_uc *pixels = stbi_load(texture_name, &width, &height, &channels, STBI_rgb_alpha);

            if (!pixels)
                throw std::runtime_error("Failed to load texture");
            if (width != m_texture_width || height != m_texture_height)
                throw std::runtime_error("Texture has incorrect dimensions");
            
            std::memcpy(mapped_data, pixels, image_size);
            stbi_image_free(pixels);
            mapped_data += m_texture_width * m_texture_height * 4;
        }

        vmaUnmapMemory(m_context.allocator, staging_buffer_allocation);

        create_image(nullptr, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0);
        transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copy_buffer_to_image(staging_buffer);
        transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vmaDestroyBuffer(m_context.allocator, staging_buffer, staging_buffer_allocation);

        VkImageViewCreateInfo view_info {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = m_image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = m_texture_layers;
        CHECK_VK(vkCreateImageView(m_context.device, &view_info, nullptr, &m_image_view));

        VkSamplerCreateInfo sampler_info {};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_NEAREST;
        sampler_info.minFilter = VK_FILTER_NEAREST;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.anisotropyEnable = false;
        sampler_info.maxAnisotropy = 1.0f;
        sampler_info.unnormalizedCoordinates = false;
        sampler_info.compareEnable = false;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        CHECK_VK(vkCreateSampler(m_context.device, &sampler_info, nullptr, &m_sampler));
    }

    void Texture::cleanup() {
        vkDestroySampler(m_context.device, m_sampler, nullptr);
        vkDestroyImageView(m_context.device, m_image_view, nullptr);
        vmaDestroyImage(m_context.allocator, m_image, m_image_allocation);
    }

    void Texture::create_image(VmaAllocationInfo *alloc_info, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaAllocationCreateFlags alloc_flags) {
        VkImageCreateInfo image_info {};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = m_texture_width;
        image_info.extent.height = m_texture_height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = m_texture_layers;
        image_info.format = format;
        image_info.tiling = tiling;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = usage;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo alloc_create_info {};
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_create_info.flags = alloc_flags;

        CHECK_VK(vmaCreateImage(m_context.allocator, &image_info, &alloc_create_info, &m_image, &m_image_allocation, alloc_info));
    }

    void Texture::transition_image_layout(VkImageLayout old_layout, VkImageLayout new_layout) {
        VkCommandBuffer command_buffer = m_context.begin_single_time_commands();

        VkImageMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = m_texture_layers;
        
        VkPipelineStageFlags src_stage;
        VkPipelineStageFlags dst_stage;

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            util::log(util::ERROR, "Renderer/Texture", "Unsupported image layout transition");
        }

        vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        m_context.end_single_time_commands(command_buffer);
    }

    void Texture::copy_buffer_to_image(VkBuffer buffer) {
        VkCommandBuffer command_buffer = m_context.begin_single_time_commands();
        
        std::array<VkBufferImageCopy, texture_names.size()> copy_regions;
        for (u32 layer = 0; layer < copy_regions.size(); layer++) {
            auto &region = copy_regions[layer];
            region.bufferOffset = m_texture_width * m_texture_height * 4 * layer;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = layer;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { (u32)m_texture_width, (u32)m_texture_height, 1 };
        }

        vkCmdCopyBufferToImage(command_buffer, buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copy_regions.size(), copy_regions.data());

        m_context.end_single_time_commands(command_buffer);
    }
}
