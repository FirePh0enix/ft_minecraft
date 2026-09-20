struct Model
{
    model_matrix: mat4x4<f32>,
}

struct Camera
{
    view_matrix: mat4x4<f32>,
}

struct WorldEnv
{
    light_view_projection: mat4x4<f32>,
    light_dir: vec3<f32>,
}

struct VertexOutput
{
    @builtin(position) clip_position: vec4<f32>,
}

@group(0) @binding(0) var<uniform> camera: Camera;
@group(0) @binding(1) var<uniform> model: Model;
@group(0) @binding(2) var<uniform> global_model: Model;
@group(0) @binding(3) var<uniform> world_env: WorldEnv;

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    
    @builtin(vertex_index) vertex_index: u32,
) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = camera.view_matrix * global_model.model_matrix * model.model_matrix * vec4(position, 1.0);
    return out;
}
