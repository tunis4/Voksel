#include "descriptor.hpp"

namespace renderer {
    DescriptorBuilder DescriptorBuilder::begin(VkDevice device, VkDescriptorPool descriptor_pool) {
        DescriptorBuilder builder;
        builder.m_device = device;
        builder.m_descriptor_pool = descriptor_pool;
        return builder;
    }

    DescriptorBuilder& DescriptorBuilder::clear() {
        m_bindings.clear();
        m_binding_flags.clear();
        m_variable_descriptor_count = 0;
        m_layout = nullptr;
        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::bind_single(VkDescriptorType type, VkShaderStageFlags stage_flags) {
        usize binding_num = m_bindings.size();

        VkDescriptorSetLayoutBinding binding {};
        binding.binding = binding_num;
        binding.descriptorType = type;
        binding.descriptorCount = 1;
        binding.stageFlags = stage_flags;

        m_bindings.push_back(binding);
        m_binding_flags.push_back(0);

        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::bind_variable_images(uint count, VkShaderStageFlags stage_flags) {
        usize binding_num = m_bindings.size();

        VkDescriptorSetLayoutBinding binding {};
        binding.binding = binding_num;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = count;
        binding.stageFlags = stage_flags;

        m_bindings.push_back(binding);
        m_binding_flags.push_back(VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT);
        m_variable_descriptor_count = count;

        return *this;
    }

    VkDescriptorSetLayout DescriptorBuilder::create_layout() {
        VkDescriptorSetLayoutBindingFlagsCreateInfo layout_binding_flags {};
        layout_binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        layout_binding_flags.bindingCount = m_bindings.size();
        layout_binding_flags.pBindingFlags = m_binding_flags.data();

        VkDescriptorSetLayoutCreateInfo layout_info {};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pNext = &layout_binding_flags;
        layout_info.bindingCount = m_bindings.size();
        layout_info.pBindings = m_bindings.data();

        CHECK_VK(vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_layout));
        return m_layout;
    }

    DescriptorBuilder& DescriptorBuilder::write_buffer(VkBuffer buffer, usize offset, usize range) {
        usize binding_num = m_writes.size();
        usize buffer_info_index = m_buffer_infos.size();

        VkDescriptorBufferInfo buffer_info {};
        buffer_info.buffer = buffer;
        buffer_info.offset = offset;
        buffer_info.range = range;
        m_buffer_infos.push_back(buffer_info);

        VkWriteDescriptorSet descriptor_write {};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstBinding = binding_num;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = m_bindings[binding_num].descriptorType;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = (VkDescriptorBufferInfo*)(buffer_info_index + 1);
        m_writes.push_back(descriptor_write);

        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::write_image(VkImageView view, VkSampler sampler, VkImageLayout layout) {
        usize binding_num = m_writes.size();
        usize image_info_index = m_image_infos.size();

        VkDescriptorImageInfo image_info {};
        image_info.imageView = view;
        image_info.sampler = sampler;
        image_info.imageLayout = layout;
        m_image_infos.push_back(image_info);

        VkWriteDescriptorSet descriptor_write {};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstBinding = binding_num;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = m_bindings[binding_num].descriptorType;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pImageInfo = (VkDescriptorImageInfo*)(image_info_index + 1);
        m_writes.push_back(descriptor_write);

        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::push_variable_image(VkImageView view, VkSampler sampler, VkImageLayout layout) {
        VkDescriptorImageInfo image_info {};
        image_info.imageView = view;
        image_info.sampler = sampler;
        image_info.imageLayout = layout;
        m_variable_image_infos.push_back(image_info);
        return *this;
    }

    DescriptorBuilder& DescriptorBuilder::write_variable_images() {
        usize binding_num = m_writes.size();

        VkWriteDescriptorSet descriptor_write {};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstBinding = binding_num;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_write.descriptorCount = m_variable_image_infos.size();
        descriptor_write.pImageInfo = m_variable_image_infos.data();
        m_writes.push_back(descriptor_write);

        return *this;
    }

    VkDescriptorSet DescriptorBuilder::create_set() {
        VkDescriptorSetAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

        VkDescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_alloc_info {};
        if (m_variable_descriptor_count > 0) {
            variable_descriptor_count_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
            variable_descriptor_count_alloc_info.descriptorSetCount = 1;
            variable_descriptor_count_alloc_info.pDescriptorCounts = &m_variable_descriptor_count;
            alloc_info.pNext = &variable_descriptor_count_alloc_info;
        }

        alloc_info.descriptorPool = m_descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_layout;

        VkDescriptorSet set;
        CHECK_VK(vkAllocateDescriptorSets(m_device, &alloc_info, &set));

        for (auto &write : m_writes) {
            write.dstSet = set;
            if (write.pBufferInfo != 0)
                write.pBufferInfo = &m_buffer_infos[((usize)write.pBufferInfo) - 1];
            if (write.descriptorCount == 1 && write.pImageInfo != 0)
                write.pImageInfo = &m_image_infos[((usize)write.pImageInfo) - 1];
        }
        vkUpdateDescriptorSets(m_device, m_writes.size(), m_writes.data(), 0, nullptr);

        m_buffer_infos.clear();
        m_variable_image_infos.clear();
        m_writes.clear();

        return set;
    }
}
