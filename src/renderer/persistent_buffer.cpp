#include "persistent_buffer.hpp"

#include <cstring>

namespace renderer {
    void PersistentBuffer::create(Context &context, usize size, usize staging_size, VkBufferUsageFlags usage, Mode mode) {
        VmaAllocationCreateFlags alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        if (mode & READ) alloc_flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        else alloc_flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VkBufferUsageFlags actual_usage = usage;
        if (mode & READ) actual_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (mode & WRITE) actual_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationInfo alloc_info;
        context.create_buffer(&m_buffer, &m_allocation, &alloc_info, size, actual_usage, alloc_flags);

        VkMemoryPropertyFlags mem_properties;
        vmaGetAllocationMemoryProperties(context.allocator, m_allocation, &mem_properties);

        if (mem_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            // allocation ended up in mappable memory and is already mapped, we can write to it directly
            m_mapped = alloc_info.pMappedData;

            m_staging_buffer = nullptr;
            m_staging_allocation = nullptr;
            m_staging_mapped = nullptr;
        } else {
            // allocation ended up in non-mappable memory, staging buffer must be used
            VmaAllocationCreateFlags staging_alloc_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
            if (mode & READ) staging_alloc_flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            else staging_alloc_flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

            VmaAllocationInfo staging_alloc_info;
            context.create_buffer(&m_staging_buffer, &m_staging_allocation, &staging_alloc_info, staging_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging_alloc_flags);
            m_staging_mapped = staging_alloc_info.pMappedData;
        }

        fill(context, 0, size, 0);
    }

    void PersistentBuffer::destroy(Context &context) {
        m_mapped = nullptr;
        vmaDestroyBuffer(context.allocator, m_buffer, m_allocation);

        if (m_staging_buffer) {
            m_staging_mapped = nullptr;
            vmaDestroyBuffer(context.allocator, m_staging_buffer, m_staging_allocation);
        }
    }

    void PersistentBuffer::upload(Context &context, void *data, usize size, usize offset) {
        void *target = m_staging_buffer ? m_staging_mapped : (u8*)m_mapped + offset;
        std::memcpy(target, data, size);
        if (m_staging_buffer) {
            vmaFlushAllocation(context.allocator, m_staging_allocation, 0, size);

            //vkCmdPipelineBarrier: VK_ACCESS_HOST_WRITE_BIT --> VK_ACCESS_TRANSFER_READ_BIT

            VkCommandBuffer cmd = context.begin_single_time_commands();

            VkBufferCopy buffer_copy {};
            buffer_copy.srcOffset = 0;
            buffer_copy.dstOffset = offset;
            buffer_copy.size = size;
            vkCmdCopyBuffer(cmd, m_staging_buffer, m_buffer, 1, &buffer_copy);

            context.end_single_time_commands(cmd);
        }
    }

    void PersistentBuffer::fill(Context &context, u32 data, usize size, usize offset) {
        if (!m_staging_buffer) {
            std::memset((u8*)m_mapped + offset, data, size);
        } else {
            //vkCmdPipelineBarrier: VK_ACCESS_HOST_WRITE_BIT --> VK_ACCESS_TRANSFER_READ_BIT
            VkCommandBuffer cmd = context.begin_single_time_commands();
            vkCmdFillBuffer(cmd, m_buffer, offset, size, data);
            context.end_single_time_commands(cmd);
        }
    }

    void PersistentBuffer::read(Context &context, void *data, usize size, usize offset) {
        if (!m_staging_buffer) {
            std::memcpy(data, (u8*)m_mapped + offset, size);
        } else {
            VkCommandBuffer cmd = context.begin_single_time_commands();
            VkBufferCopy buffer_copy {};
            buffer_copy.srcOffset = offset;
            buffer_copy.dstOffset = 0;
            buffer_copy.size = size;
            vkCmdCopyBuffer(cmd, m_buffer, m_staging_buffer, 1, &buffer_copy);
            context.end_single_time_commands(cmd);

            std::memcpy(m_staging_mapped, data, size);
        }
    }
}
