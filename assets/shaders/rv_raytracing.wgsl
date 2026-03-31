#include "./rv_debug.wgsl"

struct SvtNode64 {
    data0: u32,
    data1: u32,
    data2: u32,
}

fn svtnode64_is_leaf(node: SvtNode64) -> bool {
    return (node.data0 & 1) == 1;
}

fn svtnode64_child_ptr(node: SvtNode64) -> u32 {
    return (node.data0 & 0xFFFFFFFE);
}

fn svtnode64_child_mask(node: SvtNode64) -> u64 {
    return (u64(node.data2) << 32) | u64(node.data1);
}

fn svtnode64_child_present(node: SvtNode64, child_index: u32) -> bool {
    return (svtnode64_child_mask(node) >> child_index & u64(1)) != 0;
}

struct DispatchParams {
    inv_proj_matrix: mat4x4<f32>,
    camera_origin: vec3<f32>,
}

@group(0) @binding(0) var albedo: texture_storage_2d<bgra8unorm, write>;
@group(0) @binding(1) var<uniform> params: DispatchParams;

@group(1) @binding(0) var<storage, read> node_pool: array<SvtNode64>;
@group(1) @binding(1) var<storage, read> leaf_data: array<u32>;

var<workgroup> wg_stack: array<array<u32, 11>, 64>;

@compute
@workgroup_size(8, 8)
fn main(
    @builtin(workgroup_id) workgroup_id: vec3<u32>,
    @builtin(global_invocation_id) global_invocation_id: vec3<u32>,
    @builtin(local_invocation_index) local_invocation_index: u32)
{
    let dimensions = textureDimensions(albedo);
    let screen_pos = global_invocation_id.xy;

    if (screen_pos.x >= dimensions.x || screen_pos.y >= dimensions.y) {
        return;
    }

    let ray = get_primary_ray(screen_pos, dimensions);
    let hit = raycast(ray, local_invocation_index, true);

    var pixel_color: vec4<f32>;

    if (!hit.miss) {
        pixel_color = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        pixel_color = get_sky_color(ray.dir);
    }

    textureStore(albedo, screen_pos, pixel_color);
}

fn get_sky_color(dir: vec3<f32>) -> vec4<f32> {
    return vec4(42.0 / 255.0, 98.0 / 255.0, 154.0 / 255.0, 1.0);
}

struct Ray {
    origin: vec3<f32>,
    dir: vec3<f32>,
}

fn get_primary_ray(screen_pos: vec2<u32>, screen_size: vec2<u32>) -> Ray {
    var ray: Ray;

    let uv = (vec2<f32>(screen_pos) / vec2<f32>(screen_size)) * 2.0 - 1.0;

    var far= params.inv_proj_matrix * vec4<f32>(uv, 1, 1);
    ray.dir = normalize(far.xyz / far.w);
    ray.origin = params.camera_origin;

    return ray;
}

struct HitResult {
    miss: bool,
    color_index: u32,

    pos: vec3<f32>,
    normal: vec3<f32>,
}

fn popcnt_var64(mask: u64, width: u32) -> u32 {
    return u32(countOneBits(mask & ((u64(1) << width) - 1)));
    // uint himask = uint(mask);
    // uint count = 0;

    // if (width >= 32) {
    //     count = countOneBits(himask);
    //     himask = uint(mask >> 32);
    // }
    // uint m = 1u << (width & 31u);
    // count += countOneBits(himask & (m - 1u));
    // return count;
}

// Reverses `pos` from range [1.0, 2.0) to (2.0, 1.0] if `dir > 0`.
fn get_mirrored_pos(pos: vec3<f32>, dir: vec3<f32>, range_check: bool) -> vec3<f32> {
    var mirrored = bitcast<f32>(bitcast<vec3<u32>>(pos) ^ vec3<u32>(0x7FFFFF));
    // XOR-ing will only work for coords in range [1.0, 2.0),
    // fallback to subtractions if that's not the case.
    if (range_check && (any(pos < vec3(1.0)) || any(pos >= vec3(2.0)))) {
        mirrored = 3.0 - pos;
    }
    return select(mirrored, pos, dir > vec3(0));
}

fn get_node_cell_index(pos: vec3<f32>, scale_exp: u32) -> u32 {
    let cell_pos = ((bitcast<vec3<u32>>(pos) >> vec3<u32>(scale_exp)) & vec3(3));
    return cell_pos.x + cell_pos.y * 4 + cell_pos.z * 16;
}

// floor(pos / scale) * scale
fn floor_scale(pos: vec3<f32>, scale_exp: u32) -> vec3<f32> {
    let mask = ~0u << scale_exp;
    return bitcast<vec3<f32>>(bitcast<vec3<u32>>(pos) & vec3<u32>(mask)); // erase bits lower than scale
}

fn get_node_cell_index2(pos: vec3<f32>, scale: f32) -> u32 {
    var cell_pos = vec3<u32>(pos * scale);
    return cell_pos.x + cell_pos.y * 4 + cell_pos.z * 16;
}

fn raycast(ray_: Ray, group_id: u32, coarse: bool) -> HitResult {
    var ray = ray_;

    // var stack = wg_stack[group_id];

    var scale_exp: u32 = 21; // 0.25

    var scale: f32 = 16.0;
    var node_index: u32 = 0;
    var node = node_pool[node_index];

    // var mirror_mask: u32 = 0;
    // if (ray.dir.x > 0) { mirror_mask |= 3 << 0; }
    // if (ray.dir.y > 0) { mirror_mask |= 3 << 2; }
    // if (ray.dir.z > 0) { mirror_mask |= 3 << 4; }

    // ray.origin = get_mirrored_pos(ray.origin, ray.dir, true);

    var pos = ray.origin; // clamp(ray.origin, vec3(1.0f), vec3(1.9999999f));
    let inv_dir = vec3(1.0) / ray.dir;

    // var side_dist = vec3(0.0);

    for (var i = 0; i < 256; i++) {
        var child_index: u32 = get_node_cell_index2(pos, scale);

        // Descend down the tree to find a leaf node, if possible. Otherwise the loop stops.
        while (!svtnode64_is_leaf(node) && svtnode64_child_present(node, child_index)) {
            node_index = svtnode64_child_ptr(node) + popcnt_var64(svtnode64_child_mask(node), child_index);
            node = node_pool[node_index];

            scale *= 0.25;
            child_index = get_node_cell_index2(pos, scale);
        }

        // We found a leaf node, we can report this ray as a hit.
        if (svtnode64_is_leaf(node)) {
            break;
        }

        pos += ray.dir * 0.9;

        // if (coarse && i > 0 && svtnode64_is_leaf(node)) {
        //     break;
        // }

        // var child_index = get_node_cell_index(pos, scale_exp); // ^ mirror_mask;

        // while (svtnode64_child_present(node, child_index) && !svtnode64_is_leaf(node)) {
        //     // stack[scale_exp >> 1] = node_index;

        //     node_index = svtnode64_child_ptr(node) + popcnt_var64(svtnode64_child_mask(node), child_index);
        //     node = node_pool[node_index];

        //     scale_exp -= 2;
        //     child_index = get_node_cell_index(pos, scale_exp); // ^ mirror_mask;
        // }
        // if (svtnode64_child_present(node, child_index) && svtnode64_is_leaf(node)) {
        //     break;
        // }

        // var adv_scale_exp = scale_exp;
        // if (((svtnode64_child_mask(node) >> (child_index & 0x2A /* 0b101010*/)) & 0x00330033) == 0) {
        //     adv_scale_exp++;
        // }

        // var cell_min = floor_scale(pos, adv_scale_exp);
        // side_dist = (cell_min - ray.origin) * inv_dir;

        // let tmax = min(min(side_dist.x, side_dist.y), side_dist.z);
        // let neighbor_max = bitcast<vec3<u32>>(cell_min) + vec3<u32>(select(-1, (1 << adv_scale_exp) - 1, any(side_dist == vec3(tmax))));
        // pos = min(ray.origin - abs(ray.dir) * tmax, bitcast<vec3<f32>>(neighbor_max));

        // let diff_pos = bitcast<vec3<u32>>(pos) ^ bitcast<vec3<u32>>(cell_min);
        // let diff_exp = firstLeadingBit((diff_pos.x | diff_pos.y | diff_pos.z) & 0xFFAAAAAA);

        // if (diff_exp > scale_exp) {
        //     scale_exp = diff_exp;
        //     if (diff_exp > 21) {
        //         break;
        //     }

        //     node_index = stack[scale_exp >> 1];
        //     node = node_pool[node_index];
        // }
    }

    var hit: HitResult;
    hit.miss = true;
    hit.color_index = 0;

    if (svtnode64_is_leaf(node) && scale_exp <= 21) {
        // pos = get_mirrored_pos(pos, ray.dir, false);
        // let child_index = get_node_cell_index(pos, scale_exp);

        hit.miss = false;
        // hit.pos = pos;

        // let tmax = min(min(side_dist.x, side_dist.y), side_dist.z);
        // let side_mask = vec3(tmax) >= side_dist;
        // hit.normal = select(-sign(ray.dir), vec3(0.0), side_mask);
    }

    return hit;
}

/*
    64 bit arithmetics

struct _u64 {
    number: array<u32, 2>,
};

fn u64_from_u32(lo: u32) -> _u64 {
    var u: _u64;
    u.number = array(lo, 0);
    return u;
}

fn u64_from_2xu32(lo: u32, hi: u32) -> _u64 {
    var u: _u64;
    u.number = array(lo, hi);
    return u;
}

fn u64_and(_a: _u64, _b: _u64) -> _u64 {
    var r = u64_from_u32(0);
    for (var i = u32(0); i != 2; i++) {
        r.number[i] = _a.number[i] & _b.number[i];
    }
    return r;
}

/// Perform `&` operation on the lower 32 bits of the u64.
fn u64_and_lo(_n: _u64, _s: u32) -> u32 {
    return _n.number[0] & _s;
}

fn u64_lshift(_n: _u64, _s: u32) -> _u64 {
    var r = _n;
    var shiftLoops    = u32(floor(f32(_s) / 31));
    var shiftLastLoop = u32(_s % 31);
    for (var i = u32(0); i != shiftLoops; i++) {
        for (var j = u32(0); j != 1; j++) {
	        r.number[j] = (r.number[j] << 31) | (r.number[j + 1] >> 1);
	    }
        r.number[1] = r.number[1] << 31;
    }
    for (var j = u32(0); j != 1; j++) {
        r.number[j] = (r.number[j] << shiftLastLoop) | select(r.number[j + 1] >> (32 - shiftLastLoop), 0x00000000, shiftLastLoop == 0);
    }
    r.number[1] = r.number[1] << shiftLastLoop;
    return r;
}

fn u64_rshift(_n: _u64, _s: u32) -> _u64 {
    var r = _n;
    var shiftLoops    = u32(floor(f32(_s) / 31));
    var shiftLastLoop = u32(_s % 31);
    for (var i = u32(0); i != shiftLoops; i++) {
        for (var j = u32(0); j != 1; j++) {
            r.number[1 - j] = (r.number[1 - j] >> 31) | (r.number[1 - j - 1] << 1);
        }
        r.number[0] = r.number[0] >> 31;
    }
    for (var j = u32(0); j != 1; j++) {
        r.number[1 - j] = (r.number[1 - j] >> shiftLastLoop) | select(r.number[1 - j - 1] << (32 - shiftLastLoop), 0x00000000, shiftLastLoop == 0);
    }
    r.number[0] = r.number[0] >> shiftLastLoop;
    return r;
}

/// Count number of `1` bits.
fn u64_popcnt(_n: _u64) -> u32 {
    return countOneBits(_n.number[0]) + countOneBits(_n.number[1]);
}

fn u64_sub(_a: _u64, _b: _u64) -> _u64 {
    var r = u64_from_u32(0);
    var underflow = u32(0);
    for (var i = u32(0); i != 2; i++) {
        let a_low = _a.number[1 - i] & 0xFFFF;
        let b_low = _b.number[1 - i] & 0xFFFF;
        var sub_low = u32(0);
        if (a_low == 0 && underflow == 1) {
            sub_low = 65535 - b_low;
            underflow = 1;
        }
        else if (a_low - underflow >= b_low) {
            sub_low = a_low - underflow - b_low;
            underflow = 0;
        }
        else {
            sub_low = (65536 + (a_low - underflow)) - b_low;
            underflow = 1;
        }
        r.number[1 - i] |= sub_low;
        let a_high = _a.number[1 - i] >> 16;
        let b_high = _b.number[1 - i] >> 16;
        var sub_high = u32(0);
        if (a_high == 0 && underflow == 1) {
            sub_high = 65535 - b_high;
            underflow = 1;
        }
        else if (a_high - underflow >= b_high) {
            sub_high = a_high - underflow - b_high;
            underflow = 0;
        }
        else {
            sub_high = (65536 + (a_high - underflow)) - b_high;
            underflow = 1;
        }
        r.number[1 - i] |= sub_high << 16;
    }
    return r;
}

*/
