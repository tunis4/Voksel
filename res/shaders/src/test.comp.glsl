#version 450

layout(local_size_x = 64) in;

layout(set = 0, binding = 0) uniform UniformBuffer {
    mat4 view;
    mat4 projection;
    vec4 frustum_planes[6];
    vec4 frustum_points[8];
    vec2 depth_pyramid_size;
    float cam_near;
    uint max_draws;
} ub;

struct SolidIndirectDraw {
    uint vertex_count, instance_count, first_vertex, first_instance;
};

layout(std430, set = 0, binding = 1) writeonly buffer SolidIndirectBuffer {
    SolidIndirectDraw draws[];
} sib;

struct TransparentIndirectDraw {
    uint vertex_count, instance_count, first_vertex, first_instance;
    uint distance2;
};

layout(std430, set = 0, binding = 2) writeonly buffer TransparentIndirectBuffer {
    TransparentIndirectDraw draws[];
} tib;

layout(std430, set = 0, binding = 3) buffer IndirectCountBuffer {
    uint solid_count;
    uint transparent_count;
} cb;

struct ChunkInfo {
    int chunk_pos_x, chunk_pos_y, chunk_pos_z;
    int face_buffer_offset, transparent_vertex_buffer_offset, transparent_index_buffer_offset, chunk_info_buffer_offset;
    int num_faces, num_transparent_indices;
};

layout(std430, set = 0, binding = 4) readonly buffer ChunkInfoBuffer {
    ChunkInfo info[];
} cib;

layout(set = 0, binding = 5) uniform sampler2D depth_pyramid;

bool frustum_cull(vec3 box_min, vec3 box_max) {
    // check box outside/inside of frustum
    for (int i = 0; i < 6; i++) {
        if ((dot(ub.frustum_planes[i], vec4(box_min.x, box_min.y, box_min.z, 1.0f)) < 0.0) &&
            (dot(ub.frustum_planes[i], vec4(box_max.x, box_min.y, box_min.z, 1.0f)) < 0.0) &&
            (dot(ub.frustum_planes[i], vec4(box_min.x, box_max.y, box_min.z, 1.0f)) < 0.0) &&
            (dot(ub.frustum_planes[i], vec4(box_max.x, box_max.y, box_min.z, 1.0f)) < 0.0) &&
            (dot(ub.frustum_planes[i], vec4(box_min.x, box_min.y, box_max.z, 1.0f)) < 0.0) &&
            (dot(ub.frustum_planes[i], vec4(box_max.x, box_min.y, box_max.z, 1.0f)) < 0.0) &&
            (dot(ub.frustum_planes[i], vec4(box_min.x, box_max.y, box_max.z, 1.0f)) < 0.0) &&
            (dot(ub.frustum_planes[i], vec4(box_max.x, box_max.y, box_max.z, 1.0f)) < 0.0))
        {
            return true;
        }
    }

    // check frustum outside/inside box
    int sum;
    sum = 0; for (int i = 0; i < 8; i++) sum += ((ub.frustum_points[i].x > box_max.x) ? 1 : 0); if (sum == 8) return true;
    sum = 0; for (int i = 0; i < 8; i++) sum += ((ub.frustum_points[i].x < box_min.x) ? 1 : 0); if (sum == 8) return true;
    sum = 0; for (int i = 0; i < 8; i++) sum += ((ub.frustum_points[i].y > box_max.y) ? 1 : 0); if (sum == 8) return true;
    sum = 0; for (int i = 0; i < 8; i++) sum += ((ub.frustum_points[i].y < box_min.y) ? 1 : 0); if (sum == 8) return true;
    sum = 0; for (int i = 0; i < 8; i++) sum += ((ub.frustum_points[i].z > box_max.z) ? 1 : 0); if (sum == 8) return true;
    sum = 0; for (int i = 0; i < 8; i++) sum += ((ub.frustum_points[i].z < box_min.z) ? 1 : 0); if (sum == 8) return true;

    return false;
}

bool occlusion_cull(vec3 box_min, vec3 box_max) {
    vec3 box_size = box_max - box_min;
    vec3 box_corners[8] = { 
        box_min.xyz,
        box_min.xyz + vec3(box_size.x, 0, 0),
        box_min.xyz + vec3(0, box_size.y, 0),
        box_min.xyz + vec3(0, 0, box_size.z),
        box_min.xyz + vec3(box_size.xy, 0),
        box_min.xyz + vec3(0, box_size.yz),
        box_min.xyz + vec3(box_size.x, 0, box_size.z),
        box_min.xyz + box_size.xyz
    };

    float max_z = 0;
    vec2 min_clip = vec2(1e20);
    vec2 max_clip = vec2(-1e20);
    for (int i = 0; i < 8; i++) {
        // transform world space box to clip space
        vec4 clip_pos = ub.projection * ub.view * vec4(box_corners[i], 1);

        if (clip_pos.w <= 0) // if any of it is behind the camera then just consider it visible
            return false;

        clip_pos.xyz = clip_pos.xyz / clip_pos.w;
        clip_pos.xy = clamp(clip_pos.xy, -1, 1);
        clip_pos.xy = clip_pos.xy * 0.5 + 0.5;

        min_clip = min(clip_pos.xy, min_clip);
        max_clip = max(clip_pos.xy, max_clip);

        max_z = clamp(max(max_z, clip_pos.z), 0, 1);
    }

    vec4 aabb = vec4(min_clip, max_clip);
    float width = (aabb.z - aabb.x) * ub.depth_pyramid_size.x;
    float height = (aabb.w - aabb.y) * ub.depth_pyramid_size.y;

    float level = ceil(log2(max(width, height)));

    float depth1 = textureLod(depth_pyramid, aabb.xy, level).x;
    float depth2 = textureLod(depth_pyramid, aabb.zy, level).x;
    float depth3 = textureLod(depth_pyramid, aabb.xw, level).x;
    float depth4 = textureLod(depth_pyramid, aabb.zw, level).x;
    float depth = min(min(depth1, depth2), min(depth3, depth4));

    return max_z < depth;
}

void main() {
    uint id = gl_GlobalInvocationID.x;
    ChunkInfo chunk_info = cib.info[id];
    if (chunk_info.num_faces == 0 && chunk_info.num_transparent_indices == 0)
        return;

    ivec3 chunk_pos = ivec3(chunk_info.chunk_pos_x, chunk_info.chunk_pos_y, chunk_info.chunk_pos_z);
    vec3 box_min = chunk_pos * 16.0;
    vec3 box_max = (chunk_pos + 1) * 16.0;

    if (frustum_cull(box_min, box_max))
        return;

    if (occlusion_cull(box_min, box_max))
        return;

    if (chunk_info.num_faces > 0) {
        uint draw_index = atomicAdd(cb.solid_count, 1);
        sib.draws[draw_index].vertex_count = chunk_info.num_faces * 6;
        sib.draws[draw_index].instance_count = 1;
        sib.draws[draw_index].first_vertex = chunk_info.face_buffer_offset * 6;
        sib.draws[draw_index].first_instance = chunk_info.chunk_info_buffer_offset;
    }

    if (chunk_info.num_transparent_indices > 0) {
        uint draw_index = atomicAdd(cb.transparent_count, 1);
        tib.draws[draw_index].vertex_count = chunk_info.num_transparent_indices;
        tib.draws[draw_index].instance_count = 1;
        tib.draws[draw_index].first_vertex = chunk_info.transparent_index_buffer_offset;
        tib.draws[draw_index].first_instance = chunk_info.chunk_info_buffer_offset;
        tib.draws[draw_index].distance2 = 0;
    }
}
