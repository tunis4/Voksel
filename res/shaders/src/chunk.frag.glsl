#version 450

layout(location = 0) in vec3 in_view_pos;
layout(location = 1) in vec3 in_tex_coords;
layout(location = 2) in float in_ao;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform UniformBuffer {
    // for vertex shader
    mat4 view;
    mat4 projection;

    // for fragment shader
    vec3 fog_color;
    float fog_near;
    float fog_far;
} ub;

layout(binding = 1) uniform sampler2DArray tex_sampler;

void main() {
    vec4 tex_color = texture(tex_sampler, in_tex_coords);
    vec4 color = vec4(tex_color.rgb * in_ao, tex_color.a);
    float fog = smoothstep(ub.fog_near, ub.fog_far, length(in_view_pos));
    out_color = vec4(mix(color, vec4(ub.fog_color, 1.0), fog).rgb, color.a);
    // out_color = vec4(color.rgb, color.a * (1.0 - smoothstep(ub.fog_near, ub.fog_far, length(in_view_pos))));
}
