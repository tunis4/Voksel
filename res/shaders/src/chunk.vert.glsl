#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in uint in_tex_info;
layout(location = 2) in float in_ao;

layout(location = 0) out vec3 out_view_pos;
layout(location = 1) out uint out_tex_index;
layout(location = 2) out vec2 out_tex_coords;
layout(location = 3) out float out_ao;

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
    uint tex_index = in_tex_info & 0x3FFFFFFFu;
    out_tex_index = tex_index;
    out_tex_coords = tex_coords[in_tex_info >> 30];

    vec4 local_pos = vec4(in_pos, 1.0);
    vec4 world_pos = pc.model * local_pos;

    if (tex_index == 11) {
        world_pos.y += sin((ub.timer + world_pos.x) * 1.5) / 11.0f;
        world_pos.y += cos((ub.timer + world_pos.z) * 1.5) / 10.0f;
        world_pos.y -= 0.2;
    }

    vec4 view_pos = ub.view * world_pos;
    out_view_pos = view_pos.xyz;
    gl_Position = ub.projection * view_pos;

    out_ao = 1 - in_ao;
}
