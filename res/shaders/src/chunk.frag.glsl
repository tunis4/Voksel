#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in float in_view_distance;
layout(location = 1) flat in uint in_tex_index;
layout(location = 2) in vec2 in_tex_coords;
layout(location = 3) flat in vec4 in_light[4];

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

layout(set = 0, binding = 5) uniform sampler2D textures[];

const float ambient_light = 0.0;

void main() {
    vec4 light = mix(mix(in_light[2], in_light[3], in_tex_coords.x), mix(in_light[1], in_light[0], in_tex_coords.x), in_tex_coords.y);

    vec4 tex_color = texture(textures[nonuniformEXT(in_tex_index)], in_tex_coords);
    vec4 color = vec4(tex_color.rgb * min(light.rgb * light.a + ambient_light, 1.0), tex_color.a);

    if (tex_color.a == 0)
        discard;

    if (in_view_distance > ub.fog_near) {
        float fog = smoothstep(ub.fog_near, ub.fog_far, in_view_distance);
        out_color = vec4(mix(color, vec4(ub.fog_color, 1.0), fog).rgb, color.a);
    } else {
        out_color = color;
    }
}
