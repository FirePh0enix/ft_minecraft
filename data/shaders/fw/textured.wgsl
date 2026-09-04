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

@group(0) @binding(3) var texture: texture_2d<f32>;
@group(0) @binding(4) var texture_sampler: sampler;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4f,
    @location(0) uv: vec2f,
}

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = camera.view_projection * model.model * vec4f(in.position, 1.0);
    out.uv = vec2f(1.0 - in.uv.x, in.uv.y);
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4f {
    return textureSample(texture, texture_sampler, in.uv);
}
