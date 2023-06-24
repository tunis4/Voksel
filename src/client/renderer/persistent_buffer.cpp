#include "persistent_buffer.hpp"

#include <cstring>

namespace render {
    void PersistentBuffer::create(Context &context, usize size, usize max_upload_size, VkBufferUsageFlags usage, bool dedicated) {
        VmaAllocationCreateFlags alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        if (dedicated) alloc_flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        VmaAllocationInfo alloc_info;
        context.create_buffer(&m_buffer, &m_allocation, &alloc_info, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, alloc_flags);
        
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
            VmaAllocationInfo staging_alloc_info;
            context.create_buffer(&m_staging_buffer, &m_staging_allocation, &staging_alloc_info, max_upload_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
            m_staging_mapped = staging_alloc_info.pMappedData;
        }
    }

    void PersistentBuffer::destroy(Context &context) {
        m_mapped = nullptr;
        vmaDestroyBuffer(context.allocator, m_buffer, m_allocation);

        if (m_staging_buffer) {
            m_staging_mapped = nullptr;
            vmaDestroyBuffer(context.allocator, m_staging_buffer, m_staging_allocation);
        }
    }

    void PersistentBuffer::upload_data(Context &context, void *data, usize size, usize offset) {
        if (!m_staging_buffer) {
            std::memcpy((u8*)m_mapped + offset, data, size);
        } else {
            memcpy(m_staging_mapped, data, size);
            vmaFlushAllocation(context.allocator, m_staging_allocation, 0, size);
            // FIXME: this is shit
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
}
