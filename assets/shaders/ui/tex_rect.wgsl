struct Env
{
    matrix: mat4x4<f32>,
}

struct Uniforms
{
    matrix: mat4x4<f32>,
}

struct VertexInput
{
    @location(0) position: vec3<f32>,
    @location(1) uv: vec2<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) uv: vec2<f32>,
}

@group(0) @binding(0) var<uniform> env: Env;
@group(0) @binding(1) var<uniform> uniforms: Uniforms;
@group(0) @binding(2) var image: texture_2d<f32>;
@group(0) @binding(3) var image_sampler: sampler;

fn srgb2physical(color: vec3<f32>) -> vec3<f32> {
    let f = pow((color + 0.055) / 1.055, vec3<f32>(2.4));
    let t = color / 12.92;
    return select(f, t, color <= vec3<f32>(0.04045));
}

@vertex
fn vertex_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * uniforms.matrix * vec4(vertex.position, 1.0);
    out.uv = vertex.uv;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;
    var color = textureSample(image, image_sampler, uv2);
    color = vec4(srgb2physical(color.xyz), color.a);
    return color;
}
