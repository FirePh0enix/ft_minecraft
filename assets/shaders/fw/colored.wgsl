struct Model {
    model: mat4x4f,
    color: vec4f,
}

struct Camera {
    view_projection: mat4x4f,
}

struct WorldEnv {
    light_view_projection: mat4x4f,
    light_dir: vec3f,
}

@group(0) @binding(0) var<uniform> model: Model;
@group(0) @binding(1) var<uniform> camera: Camera;
@group(0) @binding(2) var<uniform> world_env: WorldEnv;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4f,
    @location(0) shadow_pos: vec3f,
    @location(1) @interpolate(flat) color: vec4f,
}

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = camera.view_projection * model.model * vec4f(in.position, 1.0);
    out.color = model.color;
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f,
    @location(1) debug: vec4f,
}

@fragment
fn fragment_main(in: VertexOutput) -> FragmentOutput {
    var out: FragmentOutput;
    out.color = in.color;
    return out;
}
