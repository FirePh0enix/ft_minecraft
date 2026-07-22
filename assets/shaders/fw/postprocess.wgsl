struct PostProcessUniforms {
 fog_color: vec4<f32>,
 fog_distance: f32,
 near: f32,
 far: f32,
 underwater: u32,
};

@group(0) @binding(0) var<uniform> uniforms: PostProcessUniforms;

@group(0) @binding(1) var surface: texture_2d<f32>;
@group(0) @binding(2) var surface_sampler: sampler;

@group(0) @binding(3) var depth: texture_depth_2d;
@group(0) @binding(4) var depth_sampler: sampler;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    let u = f32((vertex_index << 1u) & 2u);
    let v = f32(vertex_index & 2u);

    var out: VertexOutput;
    out.uv = vec2f(u, 1.0 - v);
    out.position = vec4f(u * 2.0 - 1.0, v * 2.0 - 1.0, 0.0, 1.0);
    return out;
}

fn linearize_depth(d: f32) -> f32 {
    return uniforms.near * uniforms.far / (uniforms.far + d * (uniforms.near - uniforms.far));
}

fn simpleFog(d: f32) -> f32 {
    return max(min(length(d) / uniforms.fog_distance - 0.8, 1.0), 0.0);
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(surface, surface_sampler, in.uv);
    let depth = textureSample(depth, depth_sampler, in.uv);
    let linear_depth = linearize_depth(depth); // TODO: multiply this by cosO
    var fog_factor = simpleFog(linear_depth);
    if (depth == 1.0) {
        // Fix for clouds, ideally clouds should write a depth value.
        fog_factor = 0.0;
    }
    if (uniforms.underwater == 1) {
        return mix(mix(color, uniforms.fog_color, fog_factor), vec4(0.0, 0.0, 1.0, 1.0), 0.5);
    }
    return mix(color, uniforms.fog_color, fog_factor);
}
