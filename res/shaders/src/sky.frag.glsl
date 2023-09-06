#version 450

layout(location = 0) in vec3 in_view_pos;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform UniformBuffer {
    // for vertex shader
    mat4 view;
    mat4 projection;

    // for fragment shader
    vec3 fog_color;
    float fog_near;
    float fog_far;
} ub;

layout(push_constant) uniform PushConstants {
    // for vertex shader
    mat4 model;

    // for fragment shader
    vec3 color;
} pc;

void main() {
    float fog = smoothstep(ub.fog_near, ub.fog_far, length(in_view_pos));
    vec4 color = vec4(mix(pc.color.rgb, ub.fog_color, fog), 1.0);
    // out_color = pow(color, vec4(2.2));
    out_color = color;
}
