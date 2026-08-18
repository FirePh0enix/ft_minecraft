struct Model {
    model: mat4x4f,
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
}

struct VertexOutput {
    @builtin(position) clip_position: vec4f,
}

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = camera.view_projection * model.model * vec4f(in.position, 1.0);
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f,
}

@fragment
fn fragment_main(in: VertexOutput) -> FragmentOutput {
    var out: FragmentOutput;
    out.color = vec4f(1.0, 1.0, 1.0, 1.0);
    return out;
}
