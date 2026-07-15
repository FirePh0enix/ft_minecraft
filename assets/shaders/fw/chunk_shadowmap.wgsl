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
    let model_matrix = mat4x4(1.0, 0.0, 0.0, 0.0,
			      0.0, 1.0, 0.0, 0.0,
			      0.0, 0.0, 1.0, 0.0,
			      in.chunk_pos.x, in.chunk_pos.y, in.chunk_pos.z, 1.0);
    
    var out: VertexOutput;
    out.clip_position = camera.view_projection * model_matrix * vec4f(in.position, 1.0);
    return out;
}
