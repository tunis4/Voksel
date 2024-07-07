#version 450

layout(location = 0) out float out_view_distance;
layout(location = 1) out uint out_tex_index;
layout(location = 2) out vec2 out_tex_coords;
layout(location = 3) out vec4 out_light[4];

layout(set = 0, binding = 0) uniform UniformBuffer {
    // for vertex shader
    mat4 view;
    mat4 projection;

    // for fragment shader
    vec3 fog_color;
    float fog_near;
    float fog_far;

    // for both shaders
    float timer;
} ub;

struct ChunkInfo {
    int chunk_pos_x, chunk_pos_y, chunk_pos_z;
    int face_buffer_offset, transparent_vertex_buffer_offset, transparent_index_buffer_offset, chunk_info_buffer_offset;
    int num_faces, num_transparent_indices;
};

layout(std430, set = 0, binding = 1) readonly buffer ChunkInfoBuffer {
    ChunkInfo info[];
} cib;

struct Vertex {
    float pos_x, pos_y, pos_z;
    uint tex;
    uint light;
};

layout(std430, set = 0, binding = 3) readonly buffer VertexBuffer {
    Vertex data[];
} vb;

layout(std430, set = 0, binding = 4) readonly buffer IndexBuffer {
    uint data[];
} ib;

const vec2 tex_coords[4] = vec2[4](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 0),
    vec2(1, 1)
);

void main() {
    ChunkInfo chunk_info = cib.info[gl_InstanceIndex];
    vec3 chunk_pos = vec3(ivec3(chunk_info.chunk_pos_x, chunk_info.chunk_pos_y, chunk_info.chunk_pos_z) * 16);

    uint index = ib.data[gl_VertexIndex] + chunk_info.transparent_vertex_buffer_offset;
    Vertex vertex = vb.data[index];
    vec3 vertex_pos = vec3(vertex.pos_x, vertex.pos_y, vertex.pos_z);

    out_tex_index = vertex.tex & 0x3FFFFFFFu;
    out_tex_coords = tex_coords[vertex.tex >> 30];

    vec3 world_pos = chunk_pos + vertex_pos;

    if (out_tex_index == 12) {
        world_pos.y += sin((ub.timer + world_pos.x * world_pos.z) * 1.5) / 22.0;
        world_pos.y += cos((ub.timer + world_pos.z) * 1.5) / 20.0;
        world_pos.y -= 0.2;
    }

    vec4 view_pos = ub.view * vec4(world_pos, 1.0);
    out_view_distance = length(view_pos.xyz);
    gl_Position = ub.projection * view_pos;

    for (uint i = 0; i < 4; i++) {
        out_light[i] = vec4(vertex.light & 0x3F, (vertex.light >> 6) & 0x3F, (vertex.light >> 12) & 0x3F, (vertex.light >> 18) & 0x3F) / 60.0;
    }
}
