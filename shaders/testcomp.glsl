#version 450

layout (local_size_x = 1) in;

// The input tensors bind index is relative to index in parameter passed
layout(set = 0, binding = 0) buffer buf_in_a { float in_a[]; };
layout(set = 0, binding = 1) buffer buf_in_b { float in_b[]; };
layout(set = 0, binding = 2) buffer buf_out_a { uint out_a[]; };
layout(set = 0, binding = 3) buffer buf_out_b { uint out_b[]; };

// Kompute supports push constants updated on dispatch
layout(push_constant) uniform PushConstants {
    float val;
} push_const;

// Kompute also supports spec constants on initalization
layout(constant_id = 0) const float const_one = 0;

void main() {
    uint index = gl_GlobalInvocationID.x;
    out_a[index] += uint( in_a[index] * in_b[index] );
    out_b[index] += uint( const_one * push_const.val );
}