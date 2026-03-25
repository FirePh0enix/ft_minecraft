struct Environments
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
    @location(1) normal: vec3<f32>,
    @location(2) uvt: vec3<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,

    @location(0) world_position: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) normal: vec3<f32>,
};


@group(0) @binding(0) var<uniform> env: Environments;
@group(0) @binding(1) var<uniform> model: Model;

@vertex
fn vertex_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput = VertexOutput();

    out.world_position = model.model_matrix * vec4<f32>(vertex.position, 1.0);
    out.position = env.view_matrix * out.world_position;

    out.uv = vertex.uvt.xy;
    out.normal = vertex.normal;

    return out;
}


@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}
