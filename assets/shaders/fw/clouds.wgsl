struct Params {
 camera_projection: mat4x4<f32>,
 camera_rot: mat4x4<f32>,
 camera_position: vec4<f32>,
 camera_dir: vec4<f32>,
 aspect_ratio: f32,
 time: f32,
 near: f32,
 far: f32,
};

struct Camera {
    view_projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<uniform> camera: Camera;

@group(0) @binding(2) var noise_texture: texture_2d<f32>;
@group(0) @binding(3) var noise_sampler: sampler;

@group(0) @binding(4) var depth_texture: texture_depth_2d;
@group(0) @binding(5) var depth_sampler: sampler;

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    // let uv = vec2(f32((vertex_index << 1) & 2), f32(vertex_index & 2));
    let uvs = array(vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    
    var out: VertexOutput;
    out.clip_position = vec4(uvs[vertex_index], 0, 1);
    out.uv = out.clip_position.xy;
    return out;
}

// Returns a linear depth value from a sample depth.
fn linearize_depth(d: f32) -> f32 {
    return params.near * params.far / (params.far + d * (params.near - params.far));
}

struct Ray {
 origin: vec3<f32>,
 dir: vec3<f32>,
};

struct RayResult {
 intersect: bool,
 t_enter: f32,
 t_exit: f32,
};

fn noise(x: vec3<f32>) -> f32 {
    let p = floor(x);
    var f = fract(x);
    f = f * f * (3.0 - 2.0 * f);

    let uv = (p.xy + vec2(37.0, 239.0) * p.z) + f.xy;
    let tex = textureSample(noise_texture, noise_sampler, (uv + 0.5) / 256.0);
    return mix(tex.x, tex.y, f.z)  * 2.0 - 1.0;
}

fn fbm(p: vec3<f32>) -> f32 {
    var q = p + params.time * 0.5 + vec3(1.0, -0.2, -1.0);
    var g = noise(q);

    var f = 0.0;
    var scale = 0.5;
    var factor = 2.02;
    for (var i = 0; i < 6; i++) {
	f += scale * noise(q);
	q *= factor;

	factor += 0.21;
	scale *= 0.5;
    }

    return f;
}

fn clouds(f: vec3<f32>, t: vec3<f32>, samples: u32, min_distance: f32, depth: f32) -> vec4<f32> {
    let u = normalize(t - f) / f32(samples);
    var res = vec4(0.0);
    
    for (var sample: u32 = 0; sample < samples; sample++) {
	let p = f + u;
	let density = fbm(p) - length(u) * f32(sample);

	if (density > 0) {
	    var color = vec4(mix(vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, 0.0), density), density);
	    var color2 = vec4(color.rgb * color.a, color.a);
	    res += color2 * (1.0 - res.a);
	}

	if (length(p) >= depth) {
	    break;
	}
    }

    return res;
}

fn intersectRay(ray: Ray, aabb_min: vec3<f32>, aabb_max: vec3<f32>) -> RayResult {
    let inv_dir = 1.0 / ray.dir;
    let t0 = (aabb_min - ray.origin) * inv_dir;
    let t1 = (aabb_max - ray.origin) * inv_dir;

    let t_min = min(t0, t1); // per-axis entry
    let t_max = max(t0, t1); // per-axis exit

    // Overall entry is the max of per-axis entries
    let t_enter = max(max(t_min.x, t_min.y), t_min.z);
    // Overall exit is the min of per-axis exits
    let t_exit  = min(min(t_max.x, t_max.y), t_max.z);

    // Typical ray intersection constraints:
    // - intervals overlap: tEnter <= tExit
    // - intersection is in front of ray origin: tExit >= 0
    let hit = (t_enter <= t_exit) && (t_exit >= 0.0);

    return RayResult(hit, t_enter, t_exit);
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    //var uv = in.uv;
    //uv -= 0.5;
    //uv.x *= params.aspect_ratio;

    var uv = in.uv; // * 2.0 - 1.0;
    uv.x *= params.aspect_ratio;

    let fov = 70.0;
    let tan_fov = tan(fov / 2.0);
    let ray_dir = normalize(vec3(uv * tan_fov, -1)); // non-rotated fragment ray

    let depth_uv = vec2(in.uv.x * 0.5 + 0.5, 1.0 - (in.uv.y * 0.5 + 0.5));
    let depth_raw = textureSample(depth_texture, depth_sampler, depth_uv);
    let depth_linear = linearize_depth(depth_raw);

    let ro = params.camera_position.xyz;
    let rd = (params.camera_rot * vec4(ray_dir, 1.0)).xyz;

    var color = vec4(0.0);

    let ray = Ray(ro, rd);
    let clouds_size = 400.0;
    let result = intersectRay(ray, vec3(ro.x + -clouds_size, 140, ro.z + -clouds_size), vec3(ro.x + clouds_size, 160, ro.z + clouds_size));

    // let p = ro + rd * ray_res.distance;
    // let d = params.camera_projection * vec4(p, 1.0);

    // let cosA = rd.z;
    // let pdepth = d.z * cosA;

    if (result.intersect) {
	color = clouds(ro + rd * result.t_enter, ro + rd * result.t_exit, 32, result.t_enter, depth_linear);
	// color = vec4(1.0);
    }

    return color;
}
