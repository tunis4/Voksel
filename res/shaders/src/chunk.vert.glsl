#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in uint in_tex_info;
layout(location = 2) in float in_ao;

layout(location = 0) out vec3 out_view_pos;
layout(location = 1) out vec3 out_tex_coords;
layout(location = 2) out float out_ao;

layout(binding = 0) uniform UniformBuffer {
    // for vertex shader
    mat4 view;
    mat4 projection;

    // for fragment shader
    vec3 fog_color;
    float fog_near;
    float fog_far;
} ub;

layout(push_constant) uniform PushConstants {
	mat4 model;
} pc;

const vec2 tex_coords[4] = vec2[4](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 0),
    vec2(1, 1)
);

void main() {
    vec4 world_pos = pc.model * vec4(in_pos, 1.0);
    vec4 view_pos = ub.view * world_pos;
    gl_Position = ub.projection * view_pos;
    out_view_pos = view_pos.xyz;
    uint index = in_tex_info >> 30;
    uint layer = in_tex_info & 0x3FFFFFFFu;
    vec2 tex_coord = tex_coords[index];
    out_tex_coords = vec3(tex_coord.x, tex_coord.y, layer);
    out_ao = 1 - in_ao;
    gl_Position = ub.projection * ub.view * pc.model * vec4(in_pos, 1.0);
}
