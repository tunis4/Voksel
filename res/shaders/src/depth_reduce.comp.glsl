#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, r32f) uniform writeonly image2D out_image;
layout(set = 0, binding = 1) uniform sampler2D in_image;

layout(push_constant) uniform PushConstants {
    vec2 image_size;
} pc;

void main() {
    uvec2 pos = gl_GlobalInvocationID.xy;

    // sampler is set up to do min reduction, so this computes the minimum depth of a 2x2 texel area
    float depth = texture(in_image, (vec2(pos) + vec2(0.5)) / pc.image_size).x;

    imageStore(out_image, ivec2(pos), vec4(depth));
}
