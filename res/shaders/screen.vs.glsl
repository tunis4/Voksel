#version 330 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 tex_coords;

out vec2 pass_tex_coords;

void main() {
    pass_tex_coords = tex_coords;
    gl_Position = vec4(pos.xy, 0.0, 1.0);
}
