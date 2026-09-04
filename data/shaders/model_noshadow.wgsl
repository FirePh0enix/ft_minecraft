struct Camera {
    view_projection: mat4x4f,
}

struct WorldEnv {
    light_view_projection: mat4x4f,
    light_dir: vec3f,
}

struct Model {
    model_matrix: mat4x4f,
}

@group(0) @binding(0) var<uniform> model: Model;
@group(0) @binding(1) var<uniform> camera: Camera;
@group(0) @binding(2) var<uniform> world_env: WorldEnv;

@group(0) @binding(3) var atlas: texture_2d<f32>;
@group(0) @binding(4) var atlas_sampler: sampler;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uvtg: vec4f,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4f,
    @location(0) uv: vec2f,
    @location(1) inv_normal: vec3f,
    @location(2) normal: vec3f,
}

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.uv = vec2f(in.uvtg.x, in.uvtg.y);
    out.normal = normalize(in.normal);
    out.clip_position = camera.view_projection * model.model_matrix * vec4f(in.position, 1.0);
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(atlas, atlas_sampler, in.uv);
}
