#pragma once

#include <vector>
#include <functional>

#include "context.hpp"

namespace renderer {
    class DescriptorBuilder {
        VkDevice m_device;
        VkDescriptorPool m_descriptor_pool;

        VkDescriptorSetLayout m_layout;
        uint m_variable_descriptor_count = 0;
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
        std::vector<VkDescriptorBindingFlags> m_binding_flags;

        std::vector<VkDescriptorBufferInfo> m_buffer_infos;
        std::vector<VkDescriptorImageInfo> m_image_infos;
        std::vector<VkDescriptorImageInfo> m_variable_image_infos;
        std::vector<VkWriteDescriptorSet> m_writes;

    public:
        static DescriptorBuilder begin(VkDevice device, VkDescriptorPool descriptor_pool);
        DescriptorBuilder& clear();

        DescriptorBuilder& bind_single(VkDescriptorType type, VkShaderStageFlags stage_flags);
        DescriptorBuilder& bind_variable_images(uint count, VkShaderStageFlags stage_flags);
        VkDescriptorSetLayout create_layout();

        DescriptorBuilder& write_buffer(VkBuffer buffer, usize offset, usize range);
        DescriptorBuilder& write_image(VkImageView view, VkSampler sampler, VkImageLayout layout);
        DescriptorBuilder& push_variable_image(VkImageView view, VkSampler sampler, VkImageLayout layout);
        DescriptorBuilder& write_variable_images();
        VkDescriptorSet create_set();
    };
}
