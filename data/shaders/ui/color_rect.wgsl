struct Env {
    matrix: mat4x4f,
}

struct Uniforms {
    matrix: mat4x4f,
    color: vec4f,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
}

@group(0) @binding(0) var<uniform> env: Env;
@group(0) @binding(1) var<uniform> uniforms: Uniforms;

@vertex
fn vertex_main(@location(0) position: vec3f) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * uniforms.matrix * vec4(position, 1.0);
    out.color = uniforms.color;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4f {
    return in.color;
}
