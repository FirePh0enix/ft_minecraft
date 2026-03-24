struct Enviromnent
{
    view_matrix: mat4x4<f32>,
    time: f32,
};

struct Model
{
    model_matrix: mat4x4<f32>,
}

struct VertexInput
{
    @location(0) position: vec3<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
}

@group(0) @binding(0) var<uniform> env: Enviromnent;
@group(0) @binding(1) var<uniform> model: Model;

@vertex
fn vertex_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.view_matrix * model.model_matrix * vec4<f32>(vertex.position, 1.0);
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 1.0, 0.0, 1.0);
}
