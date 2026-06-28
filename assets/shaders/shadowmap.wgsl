struct ShadowMapCamera { 
    view_projection: mat4x4f,
}

@group(0) @binding(0) var<uniform> camera: ShadowMapCamera;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) chunk_position: vec3f, // per instance
}

@vertex
fn vertex_main(in: VertexInput) -> @builtin(position) vec4f {
    let model_matrix = mat4x4<f32>(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        in.chunk_position.x, in.chunk_position.y, in.chunk_position.z, 1
    );
    return camera.view_projection * model_matrix * vec4f(in.position, 1.0);
}

@fragment
fn fragment_main() {
}
