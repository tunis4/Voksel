#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 in_view_pos;
layout(location = 1) flat in uint in_tex_index;
layout(location = 2) in vec2 in_tex_coords;
layout(location = 3) in float in_ao;

layout(location = 0) out vec4 out_color;

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

layout(set = 0, binding = 1) uniform sampler2D textures[];

void main() {
    vec4 tex_color = texture(textures[nonuniformEXT(in_tex_index)], in_tex_coords);
    vec4 color = vec4(tex_color.rgb * min(in_ao + 0.1, 1.0), tex_color.a);

    float fog = smoothstep(ub.fog_near, ub.fog_far, length(in_view_pos));
    out_color = vec4(mix(color, vec4(ub.fog_color, 1.0), fog).rgb, color.a);
    // out_color = vec4(color.rgb, color.a * (1.0 - smoothstep(ub.fog_near, ub.fog_far, length(in_view_pos))));
}
