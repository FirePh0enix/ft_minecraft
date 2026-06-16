struct Env {
    matrix: mat4x4f,
}

struct Uniforms {
    model_matrix: mat4x4f,
    time: f32,
}

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<uniform> env: Env;

@group(0) @binding(2) var albedo_texture: texture_2d<f32>;
@group(0) @binding(3) var albedo_texture_sampler: sampler;

@group(0) @binding(4) var position_texture: texture_2d<f32>;
@group(0) @binding(5) var position_texture_sampler: sampler;

@group(0) @binding(6) var normal_texture: texture_2d<f32>;
@group(0) @binding(7) var normal_texture_sampler: sampler;

@group(0) @binding(8) var ssao_texture: texture_2d<f32>;
@group(0) @binding(9) var ssao_texture_sampler: sampler;

@group(0) @binding(10) var depth_texture: texture_2d<f32>;
@group(0) @binding(11) var depth_texture_sampler: sampler;

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * uniforms.model_matrix * vec4f(in.position, 1.0);
    out.uv = in.uv;
    return out;
}

@fragment
fn fragment_main(vertex: VertexOutput) -> @location(0) vec4f {
    var uv2 = vertex.uv;
    uv2.y = 1.0 - uv2.y;

    let albedo = textureSample(albedo_texture, albedo_texture_sampler, uv2);
    let normal = textureSample(normal_texture, normal_texture_sampler, uv2);

    let texel_size = 1.0 / vec2f(textureDimensions(ssao_texture));
    var ssao_value = 0.0;
    for (var x = -2; x <= 2; x++) {
       for (var y = -2; y <= 2; y++) {
           let offset = vec2f(f32(x), f32(y)) * texel_size;
           ssao_value += textureSample(ssao_texture, ssao_texture_sampler, uv2 + offset).r;
       }
    }
    // return albedo;
    return albedo * (ssao_value / 25.0);
    // return vec4f(1, 1, 1, 1.0) * (ssao_value / 25.0);
}
