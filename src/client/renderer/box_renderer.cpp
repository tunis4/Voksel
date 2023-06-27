#include "box_renderer.hpp"

#include <cstring>
#include <imgui/imgui.h>

namespace render {
    void BoxRenderer::init() {
        VkDescriptorSetLayoutBinding ub_layout_binding {};
        ub_layout_binding.binding = 0;
        ub_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ub_layout_binding.descriptorCount = 1;
        ub_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array bindings = std::to_array<VkDescriptorSetLayoutBinding>({ ub_layout_binding });
        VkDescriptorSetLayoutCreateInfo layout_info {};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = bindings.size();
        layout_info.pBindings = bindings.data();

        CHECK_VK(vkCreateDescriptorSetLayout(m_context.device, &layout_info, nullptr, &m_descriptor_set_layout));

        for (auto &frame : m_per_frame)
            frame.uniform_buffer.create(m_context, sizeof(UniformBuffer), sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        VkDescriptorSetAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_context.descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_set_layout;

        for (auto &frame : m_per_frame) {
            CHECK_VK(vkAllocateDescriptorSets(m_context.device, &alloc_info, &frame.descriptor_set));
            
            VkDescriptorBufferInfo buffer_info {};
            buffer_info.buffer = frame.uniform_buffer.m_buffer;
            buffer_info.offset = 0;
            buffer_info.range = sizeof(UniformBuffer);

            std::array<VkWriteDescriptorSet, 1> descriptor_writes {};

            descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[0].dstSet = frame.descriptor_set;
            descriptor_writes[0].dstBinding = 0;
            descriptor_writes[0].dstArrayElement = 0;
            descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes[0].descriptorCount = 1;
            descriptor_writes[0].pBufferInfo = &buffer_info;

            vkUpdateDescriptorSets(m_context.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
        }

        m_pipeline = PipelineBuilder::begin(m_context.device)
            .shaders("box").no_vertex_input_info()
            .input_assembly().viewport_state().rasterizer(VK_CULL_MODE_NONE)
            .multisampling().depth_stencil(true, true).color_blending(true)
            .dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .layout<PushConstants>(&m_pipeline_layout, &m_descriptor_set_layout)
            .finish(m_context.render_pass);
    }

    void BoxRenderer::cleanup() {
        vkDestroyPipeline(m_context.device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.device, m_pipeline_layout, nullptr);

        for (auto &frame : m_per_frame)
            frame.uniform_buffer.destroy(m_context);

        vkDestroyDescriptorSetLayout(m_context.device, m_descriptor_set_layout, nullptr);
    }
    
    void BoxRenderer::set_box(bool show, glm::vec3 pos) {
        m_show_box = show;
        m_box_pos = pos;
    }

    void BoxRenderer::record(VkCommandBuffer cmd, uint frame_index) {
        if (!m_show_box)
            return;
        
        PerFrame &frame = m_per_frame[frame_index];
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        UniformBuffer ub {};
        ub.view = m_context.camera->m_view_matrix;
        ub.projection = m_context.camera->m_projection_matrix;
        ub.thickness = 0.025f;
        frame.uniform_buffer.upload_data(m_context, &ub, sizeof(UniformBuffer), 0);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.descriptor_set, 0, nullptr);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, m_box_pos + 0.5f);
        model = glm::scale(model, glm::vec3(1.025f)); // + .025 to avoid z-fighting

        PushConstants push_constants;
        push_constants.model = model;
        vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push_constants);
        
        vkCmdDraw(cmd, 36, 1, 0, 0);
    }
}
