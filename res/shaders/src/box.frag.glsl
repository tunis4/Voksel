#version 450

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform UniformBuffer {
    // for vertex shader
    mat4 view;
    mat4 projection;

    // for fragment shader
    float thickness;
} ub;

void main() {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if ((in_uv.x > ub.thickness && in_uv.x < 1.0 - ub.thickness) && (in_uv.y > ub.thickness && in_uv.y < 1.0 - ub.thickness))
        discard;
    out_color = pow(color, vec4(2.2));
}
