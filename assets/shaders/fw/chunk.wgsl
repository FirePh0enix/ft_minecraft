struct Camera {
    view_projection: mat4x4f,
}

struct WorldEnv {
    light_view_projection: mat4x4f,
    light_dir: vec3f,
}

@group(0) @binding(1) var<uniform> camera: Camera;
@group(0) @binding(2) var<uniform> world_env: WorldEnv;

@group(0) @binding(3) var images: texture_2d_array<f32>;
@group(0) @binding(4) var images_sampler: sampler;

@group(0) @binding(5) var shadowmap: texture_depth_2d;
@group(0) @binding(6) var shadowmap_sampler: sampler;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uvt: vec3<f32>,

    @location(3) chunk_pos: vec3<f32>, // per instance
}

struct VertexOutput {
    @builtin(position) clip_position: vec4f,
    @location(0) uv: vec2f,
    @location(1) texture_index: u32,
    @location(2) inv_normal: vec3f,
    @location(3) normal: vec3f,
    @location(4) frag_pos_light_space: vec4f,
}

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    let model_matrix = mat4x4(1.0, 0.0, 0.0, 0.0,
			      0.0, 1.0, 0.0, 0.0,
			      0.0, 0.0, 1.0, 0.0,
			      in.chunk_pos.x, in.chunk_pos.y, in.chunk_pos.z, 1.0);
    
    var out: VertexOutput;
    out.texture_index = u32(in.uvt.z);
    out.uv = vec2f(in.uvt.x, 1.0 - in.uvt.y);
    out.normal = normalize(in.normal);

    out.clip_position = camera.view_projection * model_matrix * vec4f(in.position, 1.0);
    out.frag_pos_light_space = world_env.light_view_projection * model_matrix * vec4f(in.position, 1.0);

    return out;
}

#include "lighting.wgsl"

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(images, images_sampler, in.uv, in.texture_index);
    return lighting(color, in.normal, in.frag_pos_light_space);
}
