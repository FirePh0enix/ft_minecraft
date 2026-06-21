struct SkyUniforms {
    color: vec4f,
}

struct VertexInput {
    @builtin(vertex_index) vertex_index: u32,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}

@group(0) @binding(0) var albedo_texture: texture_2d<f32>;
@group(0) @binding(1) var albedo_texture_sampler: sampler;

@group(0) @binding(2) var<uniform> sky: SkyUniforms;

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    let u = f32((in.vertex_index << 1u) & 2u);
    let v = f32(in.vertex_index & 2u);

    var out: VertexOutput;
    out.uv = vec2f(u, 1.0 - v);
    out.position = vec4f(u * 2.0 - 1.0, v * 2.0 - 1.0, 0.0, 1.0);
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4f {
    return sky.color;
}
