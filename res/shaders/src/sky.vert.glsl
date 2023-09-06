#version 450

layout(location = 0) out vec3 out_view_pos;

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

struct Vertex {
    vec2 pos;
};

const Vertex vertices[] = {
    { { -0.5,  0.5 } },
    { { -0.5, -0.5 } },
    { {  0.5, -0.5 } },
    { { -0.5,  0.5 } },
    { {  0.5, -0.5 } },
    { {  0.5,  0.5 } }
};

void main() {
    Vertex vertex = vertices[gl_VertexIndex];
    vec4 view_pos = ub.view * pc.model * vec4(vertex.pos.x, 0.0, vertex.pos.y, 1.0);
    out_view_pos = view_pos.xyz;
    gl_Position = ub.projection * view_pos;
}
