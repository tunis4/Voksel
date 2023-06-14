#pragma once

#include <string>
#include <vector>

#include "common.hpp"

namespace render {
    struct Context;

    class PipelineBuilder {
        VkDevice m_device;
        
        VkShaderModule m_vert_shader_module, m_frag_shader_module;
        VkPipelineShaderStageCreateInfo m_shader_stages[2];
        VkPipelineVertexInputStateCreateInfo m_vertex_input_info {};
        VkPipelineInputAssemblyStateCreateInfo m_input_assembly {};
        VkPipelineViewportStateCreateInfo m_viewport_state {};
        VkPipelineRasterizationStateCreateInfo m_rasterizer {};
        VkPipelineMultisampleStateCreateInfo m_multisampling {};
        VkPipelineDepthStencilStateCreateInfo m_depth_stencil {};
        VkPipelineColorBlendAttachmentState m_color_blend_attachment {};
        VkPipelineColorBlendStateCreateInfo m_color_blending {};
        std::vector<VkDynamicState> m_dynamic_states;
        VkPipelineDynamicStateCreateInfo m_dynamic_state {};
        VkPipelineLayout m_layout;

        VkVertexInputBindingDescription *m_vertex_input_binding_description;
        VkVertexInputAttributeDescription *m_vertex_input_attribute_descriptions;
        usize m_vertex_input_num_attribute_descriptions;

        VkShaderModule create_shader_module(const std::string &filename);

    public:
        static PipelineBuilder begin(VkDevice device);
        VkPipeline finish(VkRenderPass render_pass, u32 subpass = 0);

        // if vertex shader is at "res/shaders/example.vert.glsl" then vert_shader_name is "example"
        PipelineBuilder& shaders(const std::string &vert_shader_name, const std::string &frag_shader_name);

        // same name for both vert and frag
        inline PipelineBuilder& shaders(const std::string &shader_name) {
            return shaders(shader_name, shader_name);
        }

        template<VertexFormat V>
        PipelineBuilder& vertex_input_info() {
            m_vertex_input_binding_description = V::alloc_binding_description();
            m_vertex_input_num_attribute_descriptions = V::num_attribute_descriptions;
            m_vertex_input_attribute_descriptions = V::alloc_attribute_descriptions();
            
            m_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            m_vertex_input_info.vertexBindingDescriptionCount = 1;
            m_vertex_input_info.pVertexBindingDescriptions = m_vertex_input_binding_description;
            m_vertex_input_info.vertexAttributeDescriptionCount = m_vertex_input_num_attribute_descriptions;
            m_vertex_input_info.pVertexAttributeDescriptions = m_vertex_input_attribute_descriptions;
            return *this;
        }

        PipelineBuilder& input_assembly();
        PipelineBuilder& viewport_state();
        PipelineBuilder& rasterizer(uint cull_mode = VK_CULL_MODE_BACK_BIT);
        PipelineBuilder& multisampling();
        PipelineBuilder& depth_stencil(bool depth_test, bool depth_write);
        PipelineBuilder& color_blending(bool enable_blending);
        PipelineBuilder& dynamic_states(std::vector<VkDynamicState> &&dynamic_states);
        
        PipelineBuilder& layout(VkPipelineLayout existing_layout);
        PipelineBuilder& layout(VkPipelineLayout *new_layout, VkDescriptorSetLayout *descriptor_set_layout);
        
        template<class PushConstants>
        PipelineBuilder& layout(VkPipelineLayout *new_layout, VkDescriptorSetLayout *descriptor_set_layout) {
            VkPushConstantRange push_constant_range;
            push_constant_range.offset = 0;
            push_constant_range.size = sizeof(PushConstants);
            push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            VkPipelineLayoutCreateInfo pipeline_layout_info {};
            pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_info.setLayoutCount = 1;
            pipeline_layout_info.pSetLayouts = descriptor_set_layout;
            pipeline_layout_info.pushConstantRangeCount = 1;
            pipeline_layout_info.pPushConstantRanges = &push_constant_range;

            CHECK_VK(vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_layout));
            
            if (new_layout)
                *new_layout = m_layout;
            
            return *this;
        }
    };
}
