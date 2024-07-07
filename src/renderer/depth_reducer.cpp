#include "depth_reducer.hpp"

#include <algorithm>

namespace renderer {
    void DepthReducer::init() {
        std::array pool_sizes = std::to_array<VkDescriptorPoolSize>({
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 16 * MAX_FRAMES_IN_FLIGHT },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 * MAX_FRAMES_IN_FLIGHT }
        });

        VkDescriptorPoolCreateInfo pool_info {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = pool_sizes.size();
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = 16 * MAX_FRAMES_IN_FLIGHT;
        CHECK_VK(vkCreateDescriptorPool(m_context.device, &pool_info, nullptr, &m_descriptor_pool));

        m_descriptor_builder = DescriptorBuilder::begin(m_context.device, m_descriptor_pool);
        m_descriptor_set_layout = m_descriptor_builder
            .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
            .create_layout();

        m_pipeline = PipelineBuilder::begin(m_context.device).comp_shader("depth_reduce")
            .layout<PushConstants>(&m_pipeline_layout, 1, &m_descriptor_set_layout)
            .finish_compute();

        create_descriptor_sets();
    }

    void DepthReducer::cleanup() {
        vkDestroyPipeline(m_context.device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.device, m_pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(m_context.device, m_descriptor_set_layout, nullptr);
        vkDestroyDescriptorPool(m_context.device, m_descriptor_pool, nullptr);
    }

    void DepthReducer::create_descriptor_sets() {
        auto &swapchain = m_context.swapchain;
        for (auto &frame : m_per_frame) {
            for (uint i = 0; i < swapchain.m_depth_pyramid_mip_levels; i++) {
                m_descriptor_builder.write_image(swapchain.m_depth_pyramid_views[i], swapchain.m_depth_reduction_sampler, VK_IMAGE_LAYOUT_GENERAL);
                if (i == 0)
                    m_descriptor_builder.write_image(swapchain.m_depth_image_view, swapchain.m_depth_reduction_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                else
                    m_descriptor_builder.write_image(swapchain.m_depth_pyramid_views[i - 1], swapchain.m_depth_reduction_sampler, VK_IMAGE_LAYOUT_GENERAL);
                frame.descriptor_sets.push_back(m_descriptor_builder.create_set());
            }
        }
    }

    void DepthReducer::free_descriptor_sets() {
        for (auto &frame : m_per_frame)
            frame.descriptor_sets.clear();
        vkResetDescriptorPool(m_context.device, m_descriptor_pool, 0);
    }

    void DepthReducer::record(VkCommandBuffer cmd, uint frame_index) {
        PerFrame &frame = m_per_frame[frame_index];
        auto &swapchain = m_context.swapchain;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

        VkImageMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = swapchain.m_depth_image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);

        barrier.image = swapchain.m_depth_pyramid;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        for (uint i = 0; i < swapchain.m_depth_pyramid_mip_levels; i++) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, 0, 1, &frame.descriptor_sets[i], 0, nullptr);

            u32 level_width = std::max(swapchain.m_depth_pyramid_extent.width >> i, u32(1));
            u32 level_height = std::max(swapchain.m_depth_pyramid_extent.height >> i, u32(1));

            PushConstants push_constants;
            push_constants.image_size = vec2(level_width, level_height);
            vkCmdPushConstants(cmd, m_pipeline_layout, PushConstants::stage_flags, 0, sizeof(PushConstants), &push_constants);
            vkCmdDispatch(cmd, (level_width + 16 - 1) / 16, (level_height + 16 - 1) / 16, 1);

            barrier.subresourceRange.baseMipLevel = i;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        barrier.image = swapchain.m_depth_image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
    }
}
