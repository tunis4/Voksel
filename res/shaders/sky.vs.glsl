#version 330 core

layout (location = 0) in vec2 pos;

uniform mat4 model, view, projection;

out vec3 pass_view_pos;

void main() {
    vec4 view_pos = view * model * vec4(pos.x, 10.0, pos.y, 1.0);
    gl_Position = projection * view_pos;
    pass_view_pos = view_pos.xyz;
}
