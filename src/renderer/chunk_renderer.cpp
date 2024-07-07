#include "chunk_renderer.hpp"
#include "context.hpp"
#include "chunk_mesh_builder.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "descriptor.hpp"
#include "../game.hpp"

#include <imgui/imgui.h>
#include <entt/entt.hpp>

namespace renderer {
    usize ChunkMesh::used_vram() const {
        return sizeof(ChunkInfo) + num_faces * sizeof(BlockFace) + num_transparent_vertices * sizeof(ChunkTransparentVertex) + num_transparent_indices * sizeof(u32);
    }

    void ChunkRenderer::init() {
        for (auto &frame : m_per_frame) {
            frame.uniform_buffer.create(m_context, sizeof(UniformBuffer), sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            frame.compute_uniform_buffer.create(m_context, sizeof(ComputeUniformBuffer), sizeof(ComputeUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            frame.indirect_buffer.create(m_context, indirect_buffer_size, indirect_buffer_size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            frame.indirect_count_buffer.create(m_context, 2 * sizeof(u32), 2 * sizeof(u32), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        }

        m_face_buffer.create(m_context, face_buffer_size, 2 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        m_transparent_vertex_buffer.create(m_context, transparent_vertex_buffer_size, 2 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        m_transparent_index_buffer.create(m_context, transparent_index_buffer_size, 2 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        m_chunk_info_buffer.create(m_context, sizeof(ChunkInfo) * max_draws, sizeof(ChunkInfo) * max_draws, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        auto &texture_manager = entt::locator<TextureManager>::value();
        u32 num_textures = texture_manager.num_textures();

        auto db = DescriptorBuilder::begin(m_context.device, m_context.descriptor_pool);
        m_draw_descriptor_set_layout = db
            .bind_single(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .bind_variable_images(num_textures, VK_SHADER_STAGE_FRAGMENT_BIT)
            .create_layout();

        for (auto &frame : m_per_frame) {
            db.write_buffer(frame.uniform_buffer.m_buffer, 0, sizeof(UniformBuffer));
            db.write_buffer(m_chunk_info_buffer.m_buffer, 0, sizeof(ChunkInfo) * max_draws);
            db.write_buffer(m_face_buffer.m_buffer, 0, face_buffer_size);
            db.write_buffer(m_transparent_vertex_buffer.m_buffer, 0, transparent_vertex_buffer_size);
            db.write_buffer(m_transparent_index_buffer.m_buffer, 0, transparent_index_buffer_size);
            for (uint i = 0; i < num_textures; i++)
                db.push_variable_image(texture_manager.m_textures[i]->m_image_view, texture_manager.m_textures[i]->m_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            frame.draw_descriptor_set = db.write_variable_images().create_set();
        }

        m_pipeline = PipelineBuilder::begin(m_context.device)
            .vert_shader("chunk_solid").frag_shader("chunk").no_vertex_input_info()
            .input_assembly().viewport_state().rasterizer(VK_CULL_MODE_BACK_BIT)
            .multisampling().depth_stencil(true, true).color_blending(false)
            .dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .layout(&m_pipeline_layout, 1, &m_draw_descriptor_set_layout)
            .finish_graphics(m_context.render_pass);

        m_transparent_pipeline = PipelineBuilder::begin(m_context.device)
            .vert_shader("chunk_transparent").frag_shader("chunk").no_vertex_input_info()
            .input_assembly().viewport_state().rasterizer(VK_CULL_MODE_BACK_BIT)
            .multisampling().depth_stencil(true, true).color_blending(true)
            .dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .layout(m_pipeline_layout).finish_graphics(m_context.render_pass);

        db.clear();
        m_compute_descriptor_set_layout = db
            .bind_single(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .bind_single(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)
            .create_layout();

        for (auto &frame : m_per_frame) {
            db.write_buffer(frame.compute_uniform_buffer.m_buffer, 0, sizeof(ComputeUniformBuffer));
            db.write_buffer(frame.indirect_buffer.m_buffer, 0, sizeof(ChunkIndirect) * max_draws);
            db.write_buffer(frame.indirect_buffer.m_buffer, sizeof(ChunkIndirect) * max_draws, sizeof(ChunkTransparentIndirect) * max_draws);
            db.write_buffer(frame.indirect_count_buffer.m_buffer, 0, 2 * sizeof(u32));
            db.write_buffer(m_chunk_info_buffer.m_buffer, 0, sizeof(ChunkInfo) * max_draws);
            db.write_image(m_context.swapchain.m_depth_pyramid_full_view, m_context.swapchain.m_depth_pyramid_sampler, VK_IMAGE_LAYOUT_GENERAL);
            frame.compute_descriptor_set = db.create_set();
        }

        m_compute_pipeline = PipelineBuilder::begin(m_context.device).comp_shader("test")
            .layout(&m_compute_pipeline_layout, 1, &m_compute_descriptor_set_layout)
            .finish_compute();

        VmaVirtualBlockCreateInfo block_create_info {};
        block_create_info.size = face_buffer_size / sizeof(BlockFace);
        CHECK_VK(vmaCreateVirtualBlock(&block_create_info, &m_face_buffer_block));
        block_create_info.size = transparent_vertex_buffer_size / sizeof(ChunkTransparentVertex);
        CHECK_VK(vmaCreateVirtualBlock(&block_create_info, &m_transparent_vertex_buffer_block));
        block_create_info.size = transparent_vertex_buffer_size / sizeof(u32);
        CHECK_VK(vmaCreateVirtualBlock(&block_create_info, &m_transparent_index_buffer_block));
        block_create_info.size = max_draws;
        CHECK_VK(vmaCreateVirtualBlock(&block_create_info, &m_chunk_info_buffer_block));

        for (usize i = 0; i < 128; i++)
            m_available_mesh_builders.enqueue(new ChunkMeshBuilder());

        m_used_vram = 0;
        m_last_camera_pos = Game::get()->camera()->block_pos();
        m_timer = 0.0f;
        m_render_distance = 8;
        m_pause_reiteration = false;
        m_reiteration_requested = true;
        m_render_fog = false;
        calculate_sphere_offsets();
    }

    void ChunkRenderer::cleanup() {
        ChunkMeshBuilder *mesh_builder;
        while (m_available_mesh_builders.try_dequeue(mesh_builder))
            delete mesh_builder;
        while (m_finished_mesh_builders.try_dequeue(mesh_builder))
            delete mesh_builder;

        for (auto *mesh : m_meshes)
            delete_chunk_mesh(mesh);
        m_meshes.clear();

        vmaDestroyVirtualBlock(m_face_buffer_block);
        m_face_buffer.destroy(m_context);
        vmaDestroyVirtualBlock(m_transparent_vertex_buffer_block);
        m_transparent_vertex_buffer.destroy(m_context);
        vmaDestroyVirtualBlock(m_transparent_index_buffer_block);
        m_transparent_index_buffer.destroy(m_context);
        vmaDestroyVirtualBlock(m_chunk_info_buffer_block);
        m_chunk_info_buffer.destroy(m_context);

        vkDestroyPipeline(m_context.device, m_pipeline, nullptr);
        vkDestroyPipeline(m_context.device, m_transparent_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.device, m_pipeline_layout, nullptr);
        vkDestroyPipeline(m_context.device, m_compute_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.device, m_compute_pipeline_layout, nullptr);

        for (auto &frame : m_per_frame) {
            frame.indirect_count_buffer.destroy(m_context);
            frame.indirect_buffer.destroy(m_context);
            frame.uniform_buffer.destroy(m_context);
            frame.compute_uniform_buffer.destroy(m_context);
        }

        vkDestroyDescriptorSetLayout(m_context.device, m_draw_descriptor_set_layout, nullptr);
        vkDestroyDescriptorSetLayout(m_context.device, m_compute_descriptor_set_layout, nullptr);
    }

    void ChunkRenderer::refresh_depth_pyramid_binding() {
        for (auto &frame : m_per_frame) {
            VkDescriptorImageInfo image_info {};
            image_info.imageView = m_context.swapchain.m_depth_pyramid_full_view;
            image_info.sampler = m_context.swapchain.m_depth_pyramid_sampler;
            image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet descriptor_write {};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstBinding = 5;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pImageInfo = &image_info;
            descriptor_write.dstSet = frame.compute_descriptor_set;
            vkUpdateDescriptorSets(m_context.device, 1, &descriptor_write, 0, nullptr);
        }
    }

    void ChunkRenderer::calculate_sphere_offsets() {
        m_render_sphere_offsets.clear();
        for (i32 x = -m_render_distance; x < m_render_distance; x++) {
            for (i32 y = -m_render_distance; y < m_render_distance; y++) {
                for (i32 z = -m_render_distance; z < m_render_distance; z++) {
                    if (x * x + y * y + z * z >= m_render_distance * m_render_distance) // only check in a sphere
                        continue;
                    m_render_sphere_offsets.emplace_back(x, y, z);
                }
            }
        }

        std::sort(m_render_sphere_offsets.begin(), m_render_sphere_offsets.end(), [] (const i32vec3 &a, const i32vec3 &b) {
            return a.x * a.x + a.y * a.y + a.z * a.z < b.x * b.x + b.y * b.y + b.z * b.z;
        });
    }

    void ChunkRenderer::upload_chunk_mesh(ChunkMeshBuilder *mesh_builder, ChunkMesh *mesh) {
        if (mesh_builder->m_faces.size() == 0 && mesh_builder->m_transparent_vertices.size() == 0)
            return;

        if (mesh_builder->m_faces.size() > 0) {
            VmaVirtualAllocationCreateInfo alloc_create_info {};
            alloc_create_info.size = mesh_builder->m_faces.size();
            CHECK_VK(vmaVirtualAllocate(m_face_buffer_block, &alloc_create_info, &mesh->face_buffer_allocation, &mesh->face_buffer_offset));

            // TODO: make sure its not bigger than the staging buffer 
            m_face_buffer.upload(m_context, (void*)mesh_builder->m_faces.data(), mesh_builder->m_faces.size() * sizeof(BlockFace), mesh->face_buffer_offset * sizeof(BlockFace));
            mesh->num_faces = mesh_builder->m_faces.size();
        } else {
            mesh->face_buffer_offset = -1;
            mesh->num_faces = 0;
        }

        if (mesh_builder->m_transparent_vertices.size() > 0) {
            VmaVirtualAllocationCreateInfo alloc_create_info {};
            alloc_create_info.size = mesh_builder->m_transparent_vertices.size();
            CHECK_VK(vmaVirtualAllocate(m_transparent_vertex_buffer_block, &alloc_create_info, &mesh->transparent_vertex_buffer_allocation, &mesh->transparent_vertex_buffer_offset));
            alloc_create_info.size = mesh_builder->m_transparent_indices.size();
            CHECK_VK(vmaVirtualAllocate(m_transparent_index_buffer_block, &alloc_create_info, &mesh->transparent_index_buffer_allocation, &mesh->transparent_index_buffer_offset));

            m_transparent_vertex_buffer.upload(m_context, (void*)mesh_builder->m_transparent_vertices.data(), mesh_builder->m_transparent_vertices.size() * sizeof(ChunkTransparentVertex), mesh->transparent_vertex_buffer_offset * sizeof(ChunkTransparentVertex));
            m_transparent_index_buffer.upload(m_context, (void*)mesh_builder->m_transparent_indices.data(), mesh_builder->m_transparent_indices.size() * sizeof(u32), mesh->transparent_index_buffer_offset * sizeof(u32));
            mesh->num_transparent_vertices = mesh_builder->m_transparent_vertices.size();
            mesh->num_transparent_indices = mesh_builder->m_transparent_indices.size();
        } else {
            mesh->transparent_vertex_buffer_offset = -1;
            mesh->transparent_index_buffer_offset = -1;
            mesh->num_transparent_vertices = 0;
            mesh->num_transparent_indices = 0;
        }

        VmaVirtualAllocationCreateInfo alloc_create_info {};
        alloc_create_info.size = 1;
        CHECK_VK(vmaVirtualAllocate(m_chunk_info_buffer_block, &alloc_create_info, &mesh->chunk_info_buffer_allocation, &mesh->chunk_info_buffer_offset));

        ChunkInfo chunk_info {};
        chunk_info.chunk_pos = mesh->chunk->m_chunk_pos;
        chunk_info.chunk_info_buffer_offset = mesh->chunk_info_buffer_offset;
        chunk_info.face_buffer_offset = mesh->face_buffer_offset;
        chunk_info.transparent_vertex_buffer_offset = mesh->transparent_vertex_buffer_offset;
        chunk_info.transparent_index_buffer_offset = mesh->transparent_index_buffer_offset;
        chunk_info.num_faces = mesh->num_faces;
        chunk_info.num_transparent_indices = mesh->num_transparent_indices;
        m_chunk_info_buffer.upload(m_context, &chunk_info, sizeof(ChunkInfo), mesh->chunk_info_buffer_offset * sizeof(ChunkInfo));

        m_used_vram += mesh->used_vram();
    }

    void ChunkRenderer::delete_chunk_mesh(ChunkMesh *mesh) {
        m_used_vram -= mesh->used_vram();
        m_chunk_info_buffer.fill(m_context, 0, sizeof(ChunkInfo), mesh->chunk_info_buffer_offset * sizeof(ChunkInfo));
        vmaVirtualFree(m_face_buffer_block, mesh->face_buffer_allocation);
        vmaVirtualFree(m_transparent_vertex_buffer_block, mesh->transparent_vertex_buffer_allocation);
        vmaVirtualFree(m_transparent_index_buffer_block, mesh->transparent_index_buffer_allocation);
        vmaVirtualFree(m_chunk_info_buffer_block, mesh->chunk_info_buffer_allocation);
        mesh->face_buffer_allocation = nullptr;
        mesh->chunk_info_buffer_allocation = nullptr;
        delete mesh;
    }

    void ChunkRenderer::update(f64 delta_time) {
        m_timer += (f32)delta_time;

        i64 time_spent = 0;
        ChunkMeshBuilder *mesh_builder;
        while (time_spent < 6000 && m_finished_mesh_builders.try_dequeue(mesh_builder)) {
            auto start = std::chrono::steady_clock::now();

            auto *chunk = mesh_builder->m_chunk;
            if (chunk->m_mesh != nullptr) {
                m_meshes.erase(chunk->m_mesh);
                delete_chunk_mesh(chunk->m_mesh);
            }

            auto *mesh = new ChunkMesh(chunk);
            chunk->m_mesh = mesh;
            upload_chunk_mesh(mesh_builder, mesh);
            m_available_mesh_builders.enqueue(mesh_builder);
            m_meshes.emplace(mesh);

            auto stop = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            time_spent += duration.count();
        }

        ChunkMesh *mesh;
        while (m_meshes_to_free.try_dequeue(mesh)) {
            m_meshes.erase(mesh);
            delete_chunk_mesh(mesh);
        }
    }

    void ChunkRenderer::record_compute(VkCommandBuffer cmd, uint frame_index) {
        PerFrame &frame = m_per_frame[frame_index];
        auto world = Game::get()->world();
        auto camera = Game::get()->camera();

        bool reiterate = !m_pause_reiteration && (m_reiteration_requested || i32vec3_distance_squared(camera->block_pos(), m_last_camera_pos) > Chunk::size * Chunk::size);
        i32vec3 camera_chunk_pos = signed_i32vec3_divide(camera->block_pos(), Chunk::size).first;

        if (reiterate) {
            m_reiteration_requested = false;
            m_last_camera_pos = camera->block_pos();

            for (i32vec3 &sphere_offset : m_render_sphere_offsets) {
                i32vec3 chunk_pos = sphere_offset + camera_chunk_pos;
                world->get_or_queue_chunk_with_mesh(chunk_pos);
            }
        }

        frame.indirect_count_buffer.fill(m_context, 0, 2 * sizeof(u32), 0);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_compute_pipeline);

        ComputeUniformBuffer ub {};
        ub.view = camera->m_view_matrix;
        ub.projection = camera->m_projection_matrix;
        for (uint i = 0; i < 6; i++)
            ub.frustum_planes[i] = camera->m_frustum.m_planes[i];
        for (uint i = 0; i < 8; i++)
            ub.frustum_points[i] = vec4(camera->m_frustum.m_points[i], 0.0f);
        ub.depth_pyramid_size = vec2(m_context.swapchain.m_depth_pyramid_extent.width, m_context.swapchain.m_depth_pyramid_extent.height);
        ub.cam_near = 0.01f;
        ub.max_draws = max_draws;
        frame.compute_uniform_buffer.upload(m_context, &ub, sizeof(ComputeUniformBuffer), 0);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_compute_pipeline_layout, 0, 1, &frame.compute_descriptor_set, 0, nullptr);
        static_assert(max_draws % 64 == 0);
        vkCmdDispatch(cmd, max_draws / 64, 1, 1);

        VkMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    }

    void ChunkRenderer::record_solid_pass(VkCommandBuffer cmd, uint frame_index) {
        PerFrame &frame = m_per_frame[frame_index];
        auto camera = Game::get()->camera();

        ImGui::Begin("Chunk Renderer");

        static i32 configured_render_distance = m_render_distance;
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.5f);
        ImGui::SliderInt("Render distance", &configured_render_distance, 4, 48);
        if (ImGui::Button("Apply render distance")) {
            m_render_distance = configured_render_distance;
            calculate_sphere_offsets();
            m_reiteration_requested = true;
        }

        ImGui::Checkbox("Pause reiteration", &m_pause_reiteration);
        ImGui::Checkbox("Fog", &m_render_fog);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        UniformBuffer ub {};
        ub.view = camera->m_view_matrix;
        ub.projection = camera->m_projection_matrix;
        ub.fog_color = entt::locator<renderer::Renderer>::value().m_fog_color;
        // static f32 fog_near = 352;
        // static f32 fog_far = 384;
        // ImGui::SliderFloat("Fog near", &fog_near, 32, 512);
        // ImGui::SliderFloat("Fog far", &fog_far, 32, 512);
        // ub.fog_near = fog_near;
        // ub.fog_far = fog_far;
        if (m_render_fog) {
            ub.fog_near = ((f32)m_render_distance - 2.5f) * Chunk::size;
            ub.fog_far = ((f32)m_render_distance - 2.0f) * Chunk::size;
        } else {
            ub.fog_near = 12381321;
            ub.fog_far = 12398237;
        }
        ub.timer = m_timer;
        frame.uniform_buffer.upload(m_context, &ub, sizeof(UniformBuffer), 0);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.draw_descriptor_set, 0, nullptr);
        vkCmdDrawIndirectCount(cmd, frame.indirect_buffer.m_buffer, 0, frame.indirect_count_buffer.m_buffer, 0, max_draws, sizeof(ChunkIndirect));

        // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_transparent_pipeline);
        // vkCmdDrawIndirectCount(cmd, frame.indirect_buffer.m_buffer, sizeof(ChunkIndirect) * max_draws, frame.indirect_count_buffer.m_buffer, sizeof(u32), max_draws, sizeof(ChunkTransparentIndirect));

        ImGui::Text("Used VRAM: %.1f MiB", (f64)m_used_vram / 1024.0 / 1024.0);
        ImGui::Text("%lu meshes", m_meshes.size());
        ImGui::End();
    }

    void ChunkRenderer::record_transparent_pass(VkCommandBuffer cmd, uint frame_index) {
        PerFrame &frame = m_per_frame[frame_index];
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_transparent_pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.draw_descriptor_set, 0, nullptr);
        vkCmdDrawIndirectCount(cmd, frame.indirect_buffer.m_buffer, sizeof(ChunkIndirect) * max_draws, frame.indirect_count_buffer.m_buffer, sizeof(u32), max_draws, sizeof(ChunkTransparentIndirect));
    }
}
