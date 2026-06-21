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
    @builtin(position) position: vec4f,
    @location(0) world_position: vec4f,
    @location(1) uv: vec2f,
    @location(2) normal: vec3f,
    @location(3) light_vec: vec3f,
}

@group(0) @binding(0) var image: texture_2d<f32>;
@group(0) @binding(1) var image_sampler: sampler;
@group(0) @binding(2) var<uniform> camera: Camera;

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    let model_matrix = mat4x4<f32>(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        in.chunk_position.x, in.chunk_position.y, in.chunk_position.z, 1
    );

    var out: VertexOutput;
    out.world_position = model_matrix * vec4f(in.position, 1.0);
    out.position = camera.projection_matrix * camera.view_matrix * out.world_position;
    out.uv = in.uvt.xy;
    out.normal = in.normal;
    out.light_vec = normalize(vec3f(-1.0, -1.0, 0.0));
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4f {
    return textureSample(image, image_sampler, in.uv);
}
