#include "pipeline.hpp"

#include <array>
#include <stdexcept>
#include <fstream>

namespace renderer {
    PipelineBuilder PipelineBuilder::begin(VkDevice device) {
        PipelineBuilder builder;
        builder.m_device = device;
        return builder;
    }

    void PipelineBuilder::shader_stage(Shader *shader, VkShaderStageFlagBits stage_flag, const std::string &filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Failed to open shader file");

        usize file_size = file.tellg();
        std::vector<char> buffer(file_size);
        file.seekg(0);
        file.read(buffer.data(), file_size);
        file.close();

        VkShaderModuleCreateInfo module_create_info {};
        module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_create_info.codeSize = buffer.size();
        module_create_info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

        CHECK_VK(vkCreateShaderModule(m_device, &module_create_info, nullptr, &shader->m_module));

        shader->m_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader->m_stage_info.stage = stage_flag;
        shader->m_stage_info.module = shader->m_module;
        shader->m_stage_info.pName = "main";

        if (!shader->m_specialization_map.empty()) {
            shader->m_specialization_info.mapEntryCount = shader->m_specialization_map.size();
            shader->m_specialization_info.pMapEntries = shader->m_specialization_map.data();
            shader->m_specialization_info.dataSize = shader->m_specialization_constants.size() * sizeof(u32);
            shader->m_specialization_info.pData = shader->m_specialization_constants.data();
            shader->m_stage_info.pSpecializationInfo = &shader->m_specialization_info;
        }
    }

    PipelineBuilder& PipelineBuilder::vert_shader_specialization(u32 constant_id, u32 value) {
        m_vert_shader.m_specialization_map.emplace_back(constant_id, m_vert_shader.m_specialization_constants.size() * sizeof(u32), sizeof(u32));
        m_vert_shader.m_specialization_constants.push_back(value);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::frag_shader_specialization(u32 constant_id, u32 value) {
        m_frag_shader.m_specialization_map.emplace_back(constant_id, m_frag_shader.m_specialization_constants.size() * sizeof(u32), sizeof(u32));
        m_frag_shader.m_specialization_constants.push_back(value);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::comp_shader_specialization(u32 constant_id, u32 value) {
        m_comp_shader.m_specialization_map.emplace_back(constant_id, m_comp_shader.m_specialization_constants.size() * sizeof(u32), sizeof(u32));
        m_comp_shader.m_specialization_constants.push_back(value);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::vert_shader(const std::string &vert_shader_name) {
        shader_stage(&m_vert_shader, VK_SHADER_STAGE_VERTEX_BIT, "res/shaders/build/" + vert_shader_name + ".vert.spv");
        return *this;
    }

    PipelineBuilder& PipelineBuilder::frag_shader(const std::string &frag_shader_name) {
        shader_stage(&m_frag_shader, VK_SHADER_STAGE_FRAGMENT_BIT, "res/shaders/build/" + frag_shader_name + ".frag.spv");
        return *this;
    }

    PipelineBuilder& PipelineBuilder::comp_shader(const std::string &comp_shader_name) {
        shader_stage(&m_comp_shader, VK_SHADER_STAGE_COMPUTE_BIT, "res/shaders/build/" + comp_shader_name + ".comp.spv");
        return *this;
    }

    PipelineBuilder& PipelineBuilder::no_vertex_input_info() {
        m_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::input_assembly() {
        m_input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        m_input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        m_input_assembly.primitiveRestartEnable = false;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::viewport_state() {
        m_viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        m_viewport_state.viewportCount = 1;
        m_viewport_state.scissorCount = 1;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::rasterizer(uint cull_mode) {
        m_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        m_rasterizer.depthClampEnable = false;
        m_rasterizer.rasterizerDiscardEnable = false;
        m_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        m_rasterizer.lineWidth = 1.0f;
        m_rasterizer.cullMode = cull_mode;
        m_rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        m_rasterizer.depthBiasEnable = false;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::multisampling() {
        m_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        m_multisampling.sampleShadingEnable = false;
        m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::depth_stencil(bool depth_test, bool depth_write) {
        m_depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        m_depth_stencil.depthTestEnable = depth_test;
        m_depth_stencil.depthWriteEnable = depth_write;
        m_depth_stencil.depthCompareOp = VK_COMPARE_OP_GREATER;
        m_depth_stencil.depthBoundsTestEnable = false;
        m_depth_stencil.stencilTestEnable = false;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::color_blending(bool enable_blending) {
        m_color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        m_color_blend_attachment.blendEnable = enable_blending;
        m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        m_color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        m_color_blending.logicOpEnable = false;
        m_color_blending.logicOp = VK_LOGIC_OP_COPY;
        m_color_blending.attachmentCount = 1;
        m_color_blending.pAttachments = &m_color_blend_attachment;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::dynamic_states(std::vector<VkDynamicState> &&dynamic_states) {
        m_dynamic_states = std::move(dynamic_states);
        m_dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        m_dynamic_state.dynamicStateCount = m_dynamic_states.size();
        m_dynamic_state.pDynamicStates = m_dynamic_states.data();
        return *this;
    }

    PipelineBuilder& PipelineBuilder::layout(VkPipelineLayout existing_layout) {
        m_layout = existing_layout;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::layout(VkPipelineLayout *new_layout, u32 set_layout_count, VkDescriptorSetLayout *set_layouts) {
        VkPipelineLayoutCreateInfo pipeline_layout_info {};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = set_layout_count;
        pipeline_layout_info.pSetLayouts = set_layouts;

        CHECK_VK(vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_layout));

        if (new_layout)
            *new_layout = m_layout;

        return *this;
    }

    VkPipeline PipelineBuilder::finish_graphics(VkRenderPass render_pass, u32 subpass) {
        VkGraphicsPipelineCreateInfo pipeline_info {};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        std::array shader_stages = std::to_array<VkPipelineShaderStageCreateInfo>({ m_vert_shader.m_stage_info, m_frag_shader.m_stage_info });
        pipeline_info.stageCount = shader_stages.size();
        pipeline_info.pStages = shader_stages.data();

        pipeline_info.pVertexInputState = &m_vertex_input_info;
        pipeline_info.pInputAssemblyState = &m_input_assembly;
        pipeline_info.pViewportState = &m_viewport_state;
        pipeline_info.pRasterizationState = &m_rasterizer;
        pipeline_info.pMultisampleState = &m_multisampling;
        pipeline_info.pDepthStencilState = &m_depth_stencil;
        pipeline_info.pColorBlendState = &m_color_blending;
        pipeline_info.pDynamicState = &m_dynamic_state;

        pipeline_info.layout = m_layout;
        pipeline_info.renderPass = render_pass;
        pipeline_info.subpass = subpass;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

        VkPipeline pipeline;
        CHECK_VK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));

        vkDestroyShaderModule(m_device, m_vert_shader.m_module, nullptr);
        vkDestroyShaderModule(m_device, m_frag_shader.m_module, nullptr);
        delete m_vertex_input_binding_description;
        delete[] m_vertex_input_attribute_descriptions;

        return pipeline;
    }

    VkPipeline PipelineBuilder::finish_compute() {
        VkComputePipelineCreateInfo pipeline_info {};
        pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.layout = m_layout;
        pipeline_info.stage = m_comp_shader.m_stage_info;

        VkPipeline pipeline;
        CHECK_VK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));

        vkDestroyShaderModule(m_device, m_comp_shader.m_module, nullptr);

        return pipeline;
    }
}
