#include "storage_buffer.hpp"

#include <cstring>

namespace renderer {
    void StorageBuffer::create(Context &context, usize size, usize staging_size, VkBufferUsageFlags usage, VkPipelineStageFlags stage_flags, Mode mode) {
        VkBufferUsageFlags actual_usage = usage;
        if (mode & READ) actual_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (mode & WRITE) actual_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationInfo alloc_info;
        context.create_buffer(&m_buffer, &m_allocation, &alloc_info, size, actual_usage, 0);

        VmaAllocationCreateFlags staging_alloc_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        if (mode & READ) staging_alloc_flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        else staging_alloc_flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VmaAllocationInfo staging_alloc_info;
        context.create_buffer(&m_staging_buffer, &m_staging_allocation, &staging_alloc_info, staging_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging_alloc_flags);
        m_staging_mapped = staging_alloc_info.pMappedData;

        m_stage_flags = stage_flags;
        m_size = size;

        fill(context, 0, size, 0);
    }

    void StorageBuffer::destroy(Context &context) {
        vmaDestroyBuffer(context.allocator, m_buffer, m_allocation);

        if (m_staging_buffer) {
            m_staging_mapped = nullptr;
            vmaDestroyBuffer(context.allocator, m_staging_buffer, m_staging_allocation);
        }
    }

    void StorageBuffer::upload(Context &context, void *data, usize size, usize offset) {
        std::memcpy(m_staging_mapped, data, size);

        vmaFlushAllocation(context.allocator, m_staging_allocation, 0, size);

        VkCommandBuffer cmd = context.begin_single_time_commands();

        VkBufferMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.size = size;

        barrier.buffer = m_staging_buffer;
        barrier.offset = 0;
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

        barrier.buffer = m_buffer;
        barrier.offset = offset;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, m_stage_flags, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

        VkBufferCopy buffer_copy {};
        buffer_copy.srcOffset = 0;
        buffer_copy.dstOffset = offset;
        buffer_copy.size = size;
        vkCmdCopyBuffer(cmd, m_staging_buffer, m_buffer, 1, &buffer_copy);

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, m_stage_flags, 0, 0, nullptr, 1, &barrier, 0, nullptr);

        barrier.buffer = m_staging_buffer;
        barrier.offset = 0;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

        context.end_single_time_commands(cmd);
    }

    void StorageBuffer::fill(Context &context, u32 data, usize size, usize offset) {
        VkCommandBuffer cmd = context.begin_single_time_commands();

        VkBufferMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.size = size;
        barrier.buffer = m_buffer;
        barrier.offset = offset;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, m_stage_flags, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

        vkCmdFillBuffer(cmd, m_buffer, offset, size, data);

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, m_stage_flags, 0, 0, nullptr, 1, &barrier, 0, nullptr);

        context.end_single_time_commands(cmd);
    }
}
