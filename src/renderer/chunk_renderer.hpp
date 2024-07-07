#pragma once

#include <array>
#include <unordered_set>
#include <moodycamel/concurrentqueue.h>

#include "chunk_mesh_builder.hpp"
#include "chunk_mesh.hpp"
#include "pipeline.hpp"
#include "storage_buffer.hpp"
#include "persistent_buffer.hpp"

namespace renderer {
    struct BlockFace {
        u32 face_data;
        u32 vertex_data[4];

        inline BlockFace() {};
        inline BlockFace(uint block_x, uint block_y, uint block_z, uint normal_index, u32 tex_index) {
            face_data = block_x | (block_y << 5) | (block_z << 10) | (normal_index << 15) | (tex_index << 18);
        }

        inline void set_vertex_data(uint vertex, u16 a, u16 b, u16 c, u16 d) {
            vertex_data[vertex]  = ((a >>  0 & 0xF) + (b >>  0 & 0xF) + (c >>  0 & 0xF) + (d >>  0 & 0xF)) <<  0;
            vertex_data[vertex] |= ((a >>  4 & 0xF) + (b >>  4 & 0xF) + (c >>  4 & 0xF) + (d >>  4 & 0xF)) <<  6;
            vertex_data[vertex] |= ((a >>  8 & 0xF) + (b >>  8 & 0xF) + (c >>  8 & 0xF) + (d >>  8 & 0xF)) << 12;
            vertex_data[vertex] |= ((a >> 12 & 0xF) + (b >> 12 & 0xF) + (c >> 12 & 0xF) + (d >> 12 & 0xF)) << 18;
        }
    };

    struct ChunkTransparentVertex {
        vec3 m_pos;
        u32 m_tex; // first 30 bits for texture index, last 2 bits for coordinate index
        u32 m_light;

        inline ChunkTransparentVertex() {};
        inline ChunkTransparentVertex(vec3 pos, u32 tex_coordinate_index, u32 tex_index, u16 a, u16 b, u16 c, u16 d) {
            m_pos = pos / 2.0f + 0.5f;
            m_tex = ((tex_coordinate_index & 0b11) << 30) | (tex_index & ~((u32)0b11 << 30));
            m_light  = ((a >>  0 & 0xF) + (b >>  0 & 0xF) + (c >>  0 & 0xF) + (d >>  0 & 0xF)) <<  0;
            m_light |= ((a >>  4 & 0xF) + (b >>  4 & 0xF) + (c >>  4 & 0xF) + (d >>  4 & 0xF)) <<  6;
            m_light |= ((a >>  8 & 0xF) + (b >>  8 & 0xF) + (c >>  8 & 0xF) + (d >>  8 & 0xF)) << 12;
            m_light |= ((a >> 12 & 0xF) + (b >> 12 & 0xF) + (c >> 12 & 0xF) + (d >> 12 & 0xF)) << 18;
        }
    };

    struct ChunkInfo {
        i32vec3 chunk_pos;
        i32 face_buffer_offset;
        i32 transparent_vertex_buffer_offset;
        i32 transparent_index_buffer_offset;
        i32 chunk_info_buffer_offset;
        i32 num_faces;
        i32 num_transparent_indices;
    };

    struct ChunkIndirect {
        u32 vertex_count;
        u32 instance_count;
        u32 first_vertex;
        u32 first_instance;
    };

    struct ChunkTransparentIndirect {
        u32 vertex_count;
        u32 instance_count;
        u32 first_vertex;
        u32 first_instance;
        u32 distance2;
    };

    class ChunkMeshBuilder;
    class ChunkRenderer {
        struct UniformBuffer {
            // for vertex shader
            alignas(16) mat4 view;
            alignas(16) mat4 projection;

            // for fragment shader
            alignas(16) vec3 fog_color;
            alignas( 4) f32 fog_near;
            alignas( 4) f32 fog_far;

            // for both shaders
            alignas( 4) f32 timer;
        };

        struct ComputeUniformBuffer {
            alignas(16) mat4 view;
            alignas(16) mat4 projection;
            alignas(16) vec4 frustum_planes[6];
            alignas(16) vec4 frustum_points[8];
            alignas( 8) vec2 depth_pyramid_size;
            alignas( 4) f32 cam_near;
            alignas( 4) u32 max_draws;
        };

        struct PerFrame {
            VkDescriptorSet draw_descriptor_set;
            VkDescriptorSet compute_descriptor_set;
            PersistentBuffer uniform_buffer;
            PersistentBuffer compute_uniform_buffer;

            PersistentBuffer indirect_buffer;
            PersistentBuffer indirect_count_buffer;
        };

        std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> m_per_frame;
        Context &m_context;

    public:
        static constexpr u32 max_draws = 256 * 1024;
        static constexpr usize face_buffer_size = 512 * 1024 * 1024;
        static constexpr usize transparent_vertex_buffer_size = 1024 * 1024 * 32 * sizeof(ChunkTransparentVertex);
        static constexpr usize transparent_index_buffer_size = 1024 * 1024 * 48 * sizeof(u32);
        static constexpr usize indirect_buffer_size = sizeof(ChunkIndirect) * max_draws + sizeof(ChunkTransparentIndirect) * max_draws;

        VkDescriptorSetLayout m_draw_descriptor_set_layout;
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_transparent_pipeline;

        VkDescriptorSetLayout m_compute_descriptor_set_layout;
        VkPipeline m_compute_pipeline;
        VkPipelineLayout m_compute_pipeline_layout;

        StorageBuffer m_face_buffer;
        VmaVirtualBlock m_face_buffer_block;
        StorageBuffer m_transparent_vertex_buffer;
        VmaVirtualBlock m_transparent_vertex_buffer_block;
        StorageBuffer m_transparent_index_buffer;
        VmaVirtualBlock m_transparent_index_buffer_block;

        StorageBuffer m_chunk_info_buffer;
        VmaVirtualBlock m_chunk_info_buffer_block;

        ConcurrentQueue<ChunkMeshBuilder*> m_available_mesh_builders;
        ConcurrentQueue<ChunkMeshBuilder*> m_finished_mesh_builders;
        ConcurrentQueue<ChunkMesh*> m_meshes_to_free;

        usize m_used_vram;
        f32 m_timer;
        i32 m_render_distance;
        bool m_pause_reiteration;
        bool m_reiteration_requested;
        bool m_render_fog;
        i32vec3 m_last_camera_pos;
        std::vector<i32vec3> m_render_sphere_offsets;
        std::unordered_set<ChunkMesh*> m_meshes;

        ChunkRenderer(Context &context) : m_context(context) {}

        void init();
        void cleanup();
        void refresh_depth_pyramid_binding();
        void calculate_sphere_offsets();
        void upload_chunk_mesh(ChunkMeshBuilder *mesh_builder, ChunkMesh *mesh);
        void delete_chunk_mesh(ChunkMesh *mesh);
        void update(f64 delta_time);
        void record_compute(VkCommandBuffer cmd, uint frame_index);
        void record_solid_pass(VkCommandBuffer cmd, uint frame_index);
        void record_transparent_pass(VkCommandBuffer cmd, uint frame_index);
    };
}
