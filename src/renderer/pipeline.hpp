#pragma once

#include <string>
#include <vector>

#include "common.hpp"

namespace renderer {
    struct Context;

    class PipelineBuilder {
    public:
        static PipelineBuilder begin(VkDevice device);
        VkPipeline finish_graphics(VkRenderPass render_pass, u32 subpass = 0);
        VkPipeline finish_compute();

        PipelineBuilder& vert_shader_specialization(u32 constant_id, u32 value);
        PipelineBuilder& frag_shader_specialization(u32 constant_id, u32 value);
        PipelineBuilder& comp_shader_specialization(u32 constant_id, u32 value);

        // if vertex shader is at "res/shaders/example.vert.glsl" then vert_shader_name is "example"
        PipelineBuilder& vert_shader(const std::string &vert_shader_name);
        PipelineBuilder& frag_shader(const std::string &frag_shader_name);
        PipelineBuilder& comp_shader(const std::string &comp_shader_name);

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

        PipelineBuilder& no_vertex_input_info();

        PipelineBuilder& input_assembly();
        PipelineBuilder& viewport_state();
        PipelineBuilder& rasterizer(uint cull_mode = VK_CULL_MODE_BACK_BIT);
        PipelineBuilder& multisampling();
        PipelineBuilder& depth_stencil(bool depth_test, bool depth_write);
        PipelineBuilder& color_blending(bool enable_blending);
        PipelineBuilder& dynamic_states(std::vector<VkDynamicState> &&dynamic_states);

        PipelineBuilder& layout(VkPipelineLayout existing_layout);
        PipelineBuilder& layout(VkPipelineLayout *new_layout, u32 set_layout_count,VkDescriptorSetLayout *set_layouts);

        template<class PushConstants>
        PipelineBuilder& layout(VkPipelineLayout *new_layout, u32 set_layout_count, VkDescriptorSetLayout *set_layouts) {
            VkPushConstantRange push_constant_range;
            push_constant_range.offset = 0;
            push_constant_range.size = sizeof(PushConstants);
            push_constant_range.stageFlags = PushConstants::stage_flags;

            VkPipelineLayoutCreateInfo pipeline_layout_info {};
            pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_info.setLayoutCount = set_layout_count;
            pipeline_layout_info.pSetLayouts = set_layouts;
            pipeline_layout_info.pushConstantRangeCount = 1;
            pipeline_layout_info.pPushConstantRanges = &push_constant_range;

            CHECK_VK(vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_layout));

            if (new_layout)
                *new_layout = m_layout;

            return *this;
        }

    private:
        VkDevice m_device;

        struct Shader {
            VkShaderModule m_module;
            VkPipelineShaderStageCreateInfo m_stage_info {};
            VkSpecializationInfo m_specialization_info {};
            std::vector<VkSpecializationMapEntry> m_specialization_map;
            std::vector<u32> m_specialization_constants;
        };

        Shader m_vert_shader;
        Shader m_frag_shader;
        Shader m_comp_shader;

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

        VkVertexInputBindingDescription *m_vertex_input_binding_description = nullptr;
        VkVertexInputAttributeDescription *m_vertex_input_attribute_descriptions = nullptr;
        usize m_vertex_input_num_attribute_descriptions = 0;

        void shader_stage(Shader *shader, VkShaderStageFlagBits stage_flag, const std::string &filename);
    };
}
