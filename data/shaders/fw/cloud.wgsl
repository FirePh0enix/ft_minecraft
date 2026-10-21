struct Camera {
    view_projection: mat4x4f,
}

@group(0) @binding(1) var<uniform> camera: Camera;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) translation: vec3f,
}

@vertex
fn vertex_main(in: VertexInput) -> @builtin(position) vec4f {
    let position = in.position * vec3f(32.0, 8.0, 32.0) + in.translation;
    return camera.view_projection * vec4f(position, 1.0);
}

@fragment
fn fragment_main() -> @location(0) vec4f {
    // Preserve the scene-alpha mask that excludes clouds from SSAO.
    return vec4f(1.0, 1.0, 1.0, 0.0);
}
