struct Camera {
    view_matrix: mat4x4f,
    inv_view_matrix: mat4x4f,
    projection_matrix: mat4x4f,
}

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uvt: vec3f,
    @location(3) chunk_position: vec3f, // per instance
}

struct VertexOutput {
    @builtin(position) clip_position: vec4f,
    @location(0) uv: vec2f,
    @location(1) normal: vec3f,
    @location(2) texture_index: u32,
    @location(3) view_position: vec4f,
}

@group(0) @binding(0) var<uniform> camera: Camera;
@group(0) @binding(1) var images: texture_2d_array<f32>;
@group(0) @binding(2) var images_sampler: sampler;

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    let model_matrix = mat4x4<f32>(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        in.chunk_position.x, in.chunk_position.y, in.chunk_position.z, 1
    );

    let model_view = camera.view_matrix * model_matrix;

    var out: VertexOutput;
    out.view_position = model_view * vec4f(in.position, 1.0);
    out.clip_position = camera.projection_matrix * out.view_position;
    out.uv = in.uvt.xy;
    out.normal = (model_matrix * vec4f(in.normal, 0.0)).xyz;
    out.texture_index = u32(in.uvt.z);
    return out;
}

struct GBufferOutput {
    @location(0) albedo: vec4f,
    @location(1) position: vec4f,
    @location(2) normal: vec4f,
}

@fragment
fn fragment_main(in: VertexOutput) -> GBufferOutput {
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;

    var out: GBufferOutput;
    out.albedo = textureSample(images, images_sampler, uv2, in.texture_index);
    out.position = vec4f(in.view_position.xyz, in.clip_position.z);
    out.normal = vec4f(normalize(in.normal) * 0.5 + 0.5, 1.0);
    return out;
}
