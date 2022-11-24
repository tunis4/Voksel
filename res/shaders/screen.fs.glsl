#version 330 core

out vec4 frag_color;

in vec2 pass_tex_coords;

uniform sampler2D screen_texture;

void main() {
    vec3 col = texture(screen_texture, pass_tex_coords).rgb;
    frag_color = vec4(col, 1.0);
}
