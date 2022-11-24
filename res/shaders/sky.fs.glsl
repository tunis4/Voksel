#version 330 core

uniform vec4 color;
uniform vec4 fog_color;
uniform float fog_near;
uniform float fog_far;

in vec3 pass_view_pos;

out vec4 frag_color;

void main() {
    float fog = smoothstep(fog_near, fog_far, length(pass_view_pos));
    frag_color = vec4(mix(color, fog_color, fog).rgb, color.a);
}
