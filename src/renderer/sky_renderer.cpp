#include "sky_renderer.hpp"
#include "chunk_renderer.hpp"
#include "descriptor.hpp"
#include "renderer.hpp"
#include "../game.hpp"

#include <entt/entt.hpp>

namespace renderer {
    void SkyRenderer::init() {
        for (auto &frame : m_per_frame)
            frame.uniform_buffer.create(m_context, sizeof(UniformBuffer), sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        auto db = DescriptorBuilder::begin(m_context.device, m_context.descriptor_pool);
        m_descriptor_set_layout = db
            .bind_single(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .create_layout();

        for (auto &frame : m_per_frame) {
            db.write_buffer(frame.uniform_buffer.m_buffer, 0, sizeof(UniformBuffer));
            frame.descriptor_set = db.create_set();
        }

        m_pipeline = PipelineBuilder::begin(m_context.device)
            .vert_shader("sky").frag_shader("sky").no_vertex_input_info()
            .input_assembly().viewport_state().rasterizer(VK_CULL_MODE_NONE)
            .multisampling().depth_stencil(true, false).color_blending(true)
            .dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .layout<PushConstants>(&m_pipeline_layout, 1, &m_descriptor_set_layout)
            .finish_graphics(m_context.render_pass);
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
        auto camera = Game::get()->camera();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        UniformBuffer ub {};
        ub.view = camera->m_view_matrix;
        ub.projection = camera->m_projection_matrix;
        ub.fog_color = entt::locator<renderer::Renderer>::value().m_fog_color;
        // i32 render_distance = entt::locator<renderer::ChunkRenderer>::value().m_render_distance;
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
        frame.uniform_buffer.upload(m_context, &ub, sizeof(UniformBuffer), 0);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.descriptor_set, 0, nullptr);

        // sky plane
        mat4 model = mat4(1.0f);
        model = glm::translate(model, camera->pos() + vec3(0, 4, 0));
        model = glm::scale(model, vec3(1024, 1, 1024));

        PushConstants push_constants {};
        push_constants.model = model;
        push_constants.color = vec3(0.00143f, 0.35374f, 0.61868f);
        vkCmdPushConstants(cmd, m_pipeline_layout, PushConstants::stage_flags, 0, sizeof(PushConstants), &push_constants);

        vkCmdDraw(cmd, 6, 1, 0, 0);

        // void plane
        // model = mat4(1.0f);
        // model = glm::translate(model, m_context.camera->pos() + vec3(0, -4, 0));
        // model = glm::scale(model, vec3(1024, 1, 1024));

        // push_constants = {};
        // push_constants.model = model;
        // push_constants.color = vec3(0.0f, 0.0f, 0.0f);
        // vkCmdPushConstants(cmd, m_pipeline_layout, PushConstants::stage_flags, 0, sizeof(PushConstants), &push_constants);

        // vkCmdDraw(cmd, 6, 1, 0, 0);
    }
}
