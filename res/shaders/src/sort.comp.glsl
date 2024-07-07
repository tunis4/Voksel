#version 450

// copied from https://poniesandlight.co.uk/reflect/bitonic_merge_sort/

layout(local_size_x_id = 1) in; // Set value for local_size_x via specialization constant with id 1

#define eLocalBitonicMergeSortExample 0
#define eLocalDisperse                1
#define eBigFlip                      2
#define eBigDisperse                  3
// layout(constant_id = 0) const uint algorithm = 0;

struct TransparentIndirectDraw {
    uint vertex_count, instance_count, first_vertex, first_instance;
    uint distance2;
};

layout(set = 0, binding = 0) buffer SortData {
    // This is our unsorted input buffer - tightly packed, 
    // an array of N_GLOBAL_ELEMENTS elements.
    TransparentIndirectDraw value[];
};

layout(push_constant) uniform PushConstants {
    uint h;
    uint algorithm;
} pc;

// Workgroup local memory. We use this to minimise round-trips to global memory.
// It allows us to evaluate a sorting network of up to 1024 with one shader invocation.
shared TransparentIndirectDraw local_value[gl_WorkGroupSize.x * 2];

// naive comparison
bool is_bigger(const TransparentIndirectDraw left, const TransparentIndirectDraw right) {
    return left.distance2 > right.distance2;
}

// Pick comparison funtion: for colors we might want to compare perceptual brightness
// instead of a naive straight integer value comparison.
#define COMPARE is_bigger

void global_compare_and_swap(ivec2 idx){
    if (COMPARE(value[idx.x], value[idx.y])) {
        TransparentIndirectDraw tmp = value[idx.x];
        value[idx.x] = value[idx.y];
        value[idx.y] = tmp;
    }
}

// Performs compare-and-swap over elements held in shared,
// workgroup-local memory
void local_compare_and_swap(ivec2 idx){
    if (COMPARE(local_value[idx.x], local_value[idx.y])) {
        TransparentIndirectDraw tmp = local_value[idx.x];
        local_value[idx.x] = local_value[idx.y];
        local_value[idx.y] = tmp;
    }
}

// Performs full-height flip (h height) over globally available indices.
void big_flip(in uint h) {

    uint t_prime = gl_GlobalInvocationID.x;
    uint half_h = h >> 1; // Note: h >> 1 is equivalent to h / 2 

    uint q       = ((2 * t_prime) / h) * h;
    uint x       = q     + (t_prime % half_h);
    uint y       = q + h - (t_prime % half_h) - 1; 


    global_compare_and_swap(ivec2(x,y));
}

// Performs full-height disperse (h height) over globally available indices.
void big_disperse(uint h) {
    uint t_prime = gl_GlobalInvocationID.x;

    uint half_h = h >> 1;

    uint q = ((2 * t_prime) / h) * h;
    uint x = q + (t_prime % (half_h));
    uint y = q + (t_prime % (half_h)) + half_h;

    global_compare_and_swap(ivec2(x, y));
}

// Performs full-height flip (h height) over locally available indices.
void local_flip(uint h) {
    uint t = gl_LocalInvocationID.x;
    barrier();

    uint half_h = h >> 1; // Note: h >> 1 is equivalent to h / 2 
    ivec2 indices = 
        ivec2(h * ((2 * t) / h)) +
        ivec2(t % half_h, h - 1 - (t % half_h));

    local_compare_and_swap(indices);
}

// Performs progressively diminishing disperse operations (starting with height h)
// on locally available indices: e.g. h==8 -> 8 : 4 : 2.
// One disperse operation for every time we can divide h by 2.
void local_disperse(uint h) {
    uint t = gl_LocalInvocationID.x;
    for (; h > 1 ; h /= 2) {
        barrier();

        uint half_h = h >> 1; // Note: h >> 1 is equivalent to h / 2 
        ivec2 indices = 
            ivec2(h * ((2 * t) / h)) +
            ivec2(t % half_h, half_h + (t % half_h));

        local_compare_and_swap(indices);
    }
}

// Perform binary merge sort for local elements, up to a maximum number 
// of elements h.
void local_bms(uint h) {
    for (uint hh = 2; hh <= h; hh <<= 1) {
        local_flip(hh);
        local_disperse(hh / 2);
    }
}

void main() {
    uint t = gl_LocalInvocationID.x;
 
     // Calculate global offset for local workgroup
    uint offset = gl_WorkGroupSize.x * 2 * gl_WorkGroupID.x;

    // This shader can be called in four different modes:
    // 
    //  1. local flip+disperse (up to n == local_size_x * 2) 
    //  2. big flip
    //  3. big disperse
    //  4. local disperse 
    //
    // Which one to choose is communicated via pc.algorithm

    if (pc.algorithm <= eLocalDisperse) {
        // In case this shader executes a `local_` algorithm, we must 
        // first populate the workgroup's local memory.
        //
        local_value[t*2]   = value[offset+t*2];
        local_value[t*2+1] = value[offset+t*2+1];
    }

    switch (pc.algorithm) {
    case eLocalBitonicMergeSortExample:
        local_bms(pc.h);
        break;
    case eLocalDisperse:
        local_disperse(pc.h);
        break;
    case eBigFlip:
        big_flip(pc.h);
        break;
    case eBigDisperse:
        big_disperse(pc.h);
        break;
    }

    // Write local memory back to buffer in case we pulled in the first place.
    if (pc.algorithm <= eLocalDisperse) {
        barrier();
        // push to global memory
        value[offset+t*2]   = local_value[t*2];
        value[offset+t*2+1] = local_value[t*2+1];
    }
}
