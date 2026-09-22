struct Camera {
    view_projection: mat4x4f,
}

@group(0) @binding(1) var<uniform> camera: Camera;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) chunk_pos: vec3<f32>,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4f,
}

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = camera.view_projection * vec4f(in.position + in.chunk_pos, 1.0);
    return out;
}
