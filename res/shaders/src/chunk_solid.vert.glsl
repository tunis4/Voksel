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

layout(std430, set = 0, binding = 2) readonly buffer BlockFaceBuffer {
    uint data[];
} bfb;

const uint indices[6] = uint[6](
    0, 2, 1, 1, 2, 3
);

const vec2 tex_coords[4] = vec2[4](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 0),
    vec2(1, 1)
);

struct FaceLookup {
    vec3 vertex_offsets[4];
    uint tex_coord_indices[4];
    uint light_indices[4];
    vec3 normal;
};

const FaceLookup face_lookup_table[6] = FaceLookup[6](
    FaceLookup(vec3[4](
        vec3(-0.5, 0.5, -0.5),
        vec3(-0.5, 0.5,  0.5),
        vec3( 0.5, 0.5, -0.5),
        vec3( 0.5, 0.5,  0.5)
    ), uint[4](1, 3, 0, 2), uint[4](1, 0, 2, 3), vec3( 0,  1,  0)),
    FaceLookup(vec3[4](
        vec3(-0.5, -0.5, -0.5),
        vec3( 0.5, -0.5, -0.5),
        vec3(-0.5, -0.5,  0.5),
        vec3( 0.5, -0.5,  0.5)
    ), uint[4](1, 0, 3, 2), uint[4](2, 0, 1, 3), vec3( 0, -1,  0)),
    FaceLookup(vec3[4](
        vec3( 0.5, -0.5, -0.5),
        vec3( 0.5,  0.5, -0.5),
        vec3( 0.5, -0.5,  0.5),
        vec3( 0.5,  0.5,  0.5)
    ), uint[4](1, 0, 3, 2), uint[4](2, 0, 1, 3), vec3( 1,  0,  0)),
    FaceLookup(vec3[4](
        vec3(-0.5, -0.5, -0.5),
        vec3(-0.5, -0.5,  0.5),
        vec3(-0.5,  0.5, -0.5),
        vec3(-0.5,  0.5,  0.5)
    ), uint[4](1, 3, 0, 2), uint[4](1, 0, 2, 3), vec3(-1,  0,  0)),
    FaceLookup(vec3[4](
        vec3(-0.5, -0.5,  0.5),
        vec3( 0.5, -0.5,  0.5),
        vec3(-0.5,  0.5,  0.5),
        vec3( 0.5,  0.5,  0.5)
    ), uint[4](1, 3, 0, 2), uint[4](1, 0, 2, 3), vec3( 0,  0,  1)),
    FaceLookup(vec3[4](
        vec3(-0.5, -0.5, -0.5),
        vec3(-0.5,  0.5, -0.5),
        vec3( 0.5, -0.5, -0.5),
        vec3( 0.5,  0.5, -0.5)
    ), uint[4](1, 0, 3, 2), uint[4](2, 0, 1, 3), vec3( 0,  0, -1))
);

void main() {
    uint face_index = gl_VertexIndex / 6;
    uint face_vertex = indices[gl_VertexIndex % 6];
    uint buffer_index = face_index * 5;
    uint face_data = bfb.data[buffer_index];

    vec3 block_pos = vec3(face_data & 0x1F, (face_data >> 5) & 0x1F, (face_data >> 10) & 0x1F) + 0.5;
    uint normal_index = (face_data >> 15) & 0x7;
    // vec3 normal = normals[(in_pos_info >> 15) & 0x7];
    FaceLookup face_lookup = face_lookup_table[normal_index];
    vec3 local_pos = block_pos + face_lookup.vertex_offsets[face_vertex];

    ChunkInfo chunk_info = cib.info[gl_InstanceIndex];
    vec3 chunk_pos = vec3(ivec3(chunk_info.chunk_pos_x, chunk_info.chunk_pos_y, chunk_info.chunk_pos_z) * 16);
    vec3 world_pos = local_pos.xyz + chunk_pos;
    vec4 view_pos = ub.view * vec4(world_pos, 1.0);
    out_view_distance = length(view_pos.xyz);
    gl_Position = ub.projection * view_pos;

    out_tex_index = face_data >> 18;
    out_tex_coords = tex_coords[face_lookup.tex_coord_indices[face_vertex]];

    for (uint i = 0; i < 4; i++) {
        uint vertex_data = bfb.data[buffer_index + 1 + face_lookup.light_indices[i]];
        out_light[i] = vec4(vertex_data & 0x3F, (vertex_data >> 6) & 0x3F, (vertex_data >> 12) & 0x3F, (vertex_data >> 18) & 0x3F) / 60.0;
    }
}
