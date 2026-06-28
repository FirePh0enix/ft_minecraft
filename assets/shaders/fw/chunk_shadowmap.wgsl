struct Chunk {
    model: mat4x4f,
}

struct Camera {
    view_projection: mat4x4f,
}

@group(0) @binding(0) var<uniform> chunk: Chunk;
@group(0) @binding(1) var<uniform> camera: Camera;

struct VertexInput {
    @location(0) position: vec3f,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4f,
}

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.clip_position = camera.view_projection * chunk.model * vec4f(in.position, 1.0);
    return out;
}
