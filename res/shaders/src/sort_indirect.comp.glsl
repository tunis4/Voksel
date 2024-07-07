#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

struct IndirectDispatch {
    uint x, y, z;

};

void main() {
        uint draw_index = atomicAdd(cb.solid_count, 1);
        sib.draws[draw_index].vertex_count = chunk_info.num_faces * 6;
        sib.draws[draw_index].instance_count = 1;
        sib.draws[draw_index].first_vertex = chunk_info.face_buffer_offset * 6;
        sib.draws[draw_index].first_instance = chunk_info.chunk_info_buffer_offset;
}
