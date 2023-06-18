#pragma once

#include <queue>

#include "common.hpp"
#include "swapchain.hpp"
#include "../camera.hpp"

namespace render {
    struct BufferDeletion {
        VkBuffer m_buffer;
        VmaAllocation m_allocation;

        BufferDeletion(VkBuffer buffer, VmaAllocation allocation) : m_buffer(buffer), m_allocation(allocation) {}
    };

    struct Context {
        client::Window *window;
        client::Camera *camera;

        VkInstance instance;
        VkPhysicalDevice physical_device;
        VkDevice device;
        VkSurfaceKHR surface;
        VkRenderPass render_pass;
        VkCommandPool command_pool;
        VkCommandPool transient_command_pool;
        VkDescriptorPool descriptor_pool;
        VmaAllocator allocator;

        QueueManager queue_manager;
        Swapchain swapchain;

        std::queue<BufferDeletion> buffer_deletions;

        Context() : swapchain(*this) {}

        VkCommandBuffer begin_single_time_commands();
        void end_single_time_commands(VkCommandBuffer command_buffer);
        void create_buffer(VkBuffer *buffer, VmaAllocation *allocation, VmaAllocationInfo *alloc_info, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
        void create_image(VkImage *image, VmaAllocation *allocation, VmaAllocationInfo *alloc_info, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
        void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
        void transition_image_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);
        void copy_buffer_to_image(VkBuffer buffer, VkImage image, u32 width, u32 height);
    };
}
