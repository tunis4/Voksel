#include "box_renderer.hpp"
#include "descriptor.hpp"
#include "../game.hpp"

namespace renderer {
    void BoxRenderer::init() {
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
            .vert_shader("box").frag_shader("box").no_vertex_input_info()
            .input_assembly().viewport_state().rasterizer(VK_CULL_MODE_NONE)
            .multisampling().depth_stencil(true, true).color_blending(true)
            .dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .layout<PushConstants>(&m_pipeline_layout, 1, &m_descriptor_set_layout)
            .finish_graphics(m_context.render_pass);
    }

    void BoxRenderer::cleanup() {
        vkDestroyPipeline(m_context.device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.device, m_pipeline_layout, nullptr);

        for (auto &frame : m_per_frame)
            frame.uniform_buffer.destroy(m_context);

        vkDestroyDescriptorSetLayout(m_context.device, m_descriptor_set_layout, nullptr);
    }

    void BoxRenderer::set_box(bool show, vec3 pos) {
        m_show_box = show;
        m_box_pos = pos;
    }

    void BoxRenderer::record(VkCommandBuffer cmd, uint frame_index) {
        if (!m_show_box)
            return;

        PerFrame &frame = m_per_frame[frame_index];
        auto camera = Game::get()->camera();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        UniformBuffer ub {};
        ub.view = camera->m_view_matrix;
        ub.projection = camera->m_projection_matrix;
        ub.thickness = 0.025f;
        frame.uniform_buffer.upload(m_context, &ub, sizeof(UniformBuffer), 0);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.descriptor_set, 0, nullptr);

        mat4 model = mat4(1.0f);
        model = glm::translate(model, m_box_pos + 0.5f);
        model = glm::scale(model, vec3(1.025f)); // + .025 to avoid z-fighting

        PushConstants push_constants;
        push_constants.model = model;
        vkCmdPushConstants(cmd, m_pipeline_layout, PushConstants::stage_flags, 0, sizeof(PushConstants), &push_constants);

        vkCmdDraw(cmd, 36, 1, 0, 0);
    }
}
