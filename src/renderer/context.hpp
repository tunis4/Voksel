#pragma once

#include <glm/ext/vector_uint3_sized.hpp>

#include "common.hpp"
#include "swapchain.hpp"
#include "queue_manager.hpp"

namespace renderer {
    struct Context {
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

        u32 max_compute_shared_memory_size;
        u32 max_compute_work_group_invocations;
        glm::u32vec3 max_compute_work_group_size;
        float timestamp_period;

        Context() : swapchain(*this) {}

        VkCommandBuffer begin_single_time_commands();
        void end_single_time_commands(VkCommandBuffer command_buffer);
        void create_buffer(VkBuffer *buffer, VmaAllocation *allocation, VmaAllocationInfo *alloc_info, usize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
        void create_image(VkImage *image, VmaAllocation *allocation, VmaAllocationInfo *alloc_info, u32 width, u32 height, u32 mip_levels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaAllocationCreateFlags alloc_flags);
        void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, usize size);
        void transition_image_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, u32 mip_level = 0);
        void copy_buffer_to_image(VkBuffer buffer, VkImage image, u32 width, u32 height);
    };
}
