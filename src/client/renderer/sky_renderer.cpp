#include "sky_renderer.hpp"
#include "chunk_renderer.hpp"
#include "renderer.hpp"

#include <cstring>
#include <entt/entt.hpp>
#include <imgui/imgui.h>

namespace render {
    void SkyRenderer::init() {
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
            .shaders("sky").no_vertex_input_info()
            .input_assembly().viewport_state().rasterizer(VK_CULL_MODE_NONE)
            .multisampling().depth_stencil(true, false).color_blending(true)
            .dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .layout<PushConstants>(&m_pipeline_layout, &m_descriptor_set_layout)
            .finish(m_context.render_pass);
    }

    void SkyRenderer::cleanup() {
        vkDestroyPipeline(m_context.device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.device, m_pipeline_layout, nullptr);

        for (auto &frame : m_per_frame)
            frame.uniform_buffer.destroy(m_context);

        vkDestroyDescriptorSetLayout(m_context.device, m_descriptor_set_layout, nullptr);
    }

    void SkyRenderer::record(VkCommandBuffer cmd, uint frame_index) {
        PerFrame &frame = m_per_frame[frame_index];
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        UniformBuffer ub {};
        ub.view = m_context.camera->m_view_matrix;
        ub.projection = m_context.camera->m_projection_matrix;
        ub.fog_color = entt::locator<render::Renderer>::value().m_fog_color;
        // i32 render_distance = entt::locator<render::ChunkRenderer>::value().m_render_distance;
        // ub.fog_near = ((f32)render_distance - 2.5f) * 32;
        // ub.fog_far = ((f32)render_distance - 2.0f) * 32;
        static f32 fog_near = 8;
        static f32 fog_far = 24;
        // ImGui::Begin("Sky Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        // ImGui::SliderFloat("Fog near", &fog_near, 8, 64);
        // ImGui::SliderFloat("Fog far", &fog_far, 8, 64);
        // ImGui::End();
        ub.fog_near = fog_near;
        ub.fog_far = fog_far;
        frame.uniform_buffer.upload_data(m_context, &ub, sizeof(UniformBuffer), 0);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.descriptor_set, 0, nullptr);

        // sky plane
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, m_context.camera->pos() + glm::vec3(0, 4, 0));
        model = glm::scale(model, glm::vec3(1024, 1, 1024));

        PushConstants push_constants {};
        push_constants.model = model;
        push_constants.color = glm::vec3(0.00143f, 0.35374f, 0.61868f);
        vkCmdPushConstants(cmd, m_pipeline_layout, PushConstants::stage_flags, 0, sizeof(PushConstants), &push_constants);
        
        vkCmdDraw(cmd, 6, 1, 0, 0);

        // void plane
        model = glm::mat4(1.0f);
        model = glm::translate(model, m_context.camera->pos() + glm::vec3(0, -4, 0));
        model = glm::scale(model, glm::vec3(1024, 1, 1024));

        push_constants = {};
        push_constants.model = model;
        push_constants.color = glm::vec3(0.0f, 0.0f, 0.0f);
        vkCmdPushConstants(cmd, m_pipeline_layout, PushConstants::stage_flags, 0, sizeof(PushConstants), &push_constants);
        
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }
}
