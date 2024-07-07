#version 450

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D tex_sampler;

void main() {
    vec4 color = in_color * texture(tex_sampler, in_uv.st);
    // out_color = vec4(pow(color.rgb, vec3(2.2)), color.a);
    out_color = pow(color, vec4(2.2));
}
