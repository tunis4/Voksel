#version 450

layout(location = 0) in vec3 in_tex_coords;
layout(location = 1) in float in_ao;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2DArray tex_sampler;

void main() {
    vec4 tex_color = texture(tex_sampler, in_tex_coords);
    vec4 color = vec4(tex_color.rgb * in_ao, tex_color.a);
    out_color = color;
}
