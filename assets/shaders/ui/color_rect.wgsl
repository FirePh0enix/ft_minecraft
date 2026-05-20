struct Env
{
    matrix: mat4x4<f32>,
}

struct Uniforms
{
    matrix: mat4x4<f32>,
    color: vec4<f32>,
}

struct VertexInput
{
    @location(0) position: vec3<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
}

@group(0) @binding(0) var<uniform> env: Env;
@group(0) @binding(1) var<uniform> uniforms: Uniforms;

@vertex
fn vertex_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * uniforms.matrix * vec4(vertex.position, 1.0);
    out.color = uniforms.color;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return in.color;
}
