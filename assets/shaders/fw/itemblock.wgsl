struct Camera
{
    view_matrix: mat4x4<f32>,
};

struct Model
{
    model_matrix: mat4x4<f32>,
    textures: vec3<u32>,
}

struct WorldEnv {
    light_view_projection: mat4x4f,
    light_dir: vec3f,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(3) texture: u32,
    @location(4) shadow_pos: vec4<f32>,
    @location(5) normal: vec3<f32>,
}

@group(0) @binding(0) var<uniform> camera: Camera;
@group(0) @binding(1) var<uniform> model: Model;
@group(0) @binding(2) var<uniform> world_env: WorldEnv;
@group(0) @binding(3) var images: texture_2d_array<f32>;
@group(0) @binding(4) var images_sampler: sampler;

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,

    @builtin(vertex_index) index: u32,
) -> VertexOutput {
    var out: VertexOutput;
    out.position = camera.view_matrix * model.model_matrix * vec4<f32>(position, 1.0);
    out.uv = vec2(uv.x, 1.0 - uv.y);
    out.normal = normalize(normal);

    var offset: u32 = 0;
    if (index % 8 > 3) {
        offset = 16;
    }
    out.texture = (model.textures[index / 8] >> offset) & 0xFFFF;

    out.shadow_pos = world_env.light_view_projection * model.model_matrix * vec4(position, 1.0);
    
    return out;
}

// #include "lighting.wgsl"

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(images, images_sampler, in.uv, in.texture);
    // TODO: find a way to give it the shadowmap without too much pain.
    // return lighting(color, in.normal, in.shadow_pos);
    return color;
}
