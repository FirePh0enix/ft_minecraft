struct Uniforms {
    color: vec4f,
    position: vec3f,
    scale: vec2f,
}

struct Environment {
    matrix: mat4x4f,
}

@group(0) @binding(0) var<uniform> env: Environment;
@group(0) @binding(1) var<uniform> uniforms: Uniforms;
@group(0) @binding(2) var bitmap: texture_2d<f32>;
@group(0) @binding(3) var bitmap_sampler: sampler;

struct VertexInput {
    @builtin(vertex_index) vertex_index: u32,
    @location(0) pos: vec3f,

    @location(1) bounds: vec4f,
    @location(2) char_pos: vec2f,
    @location(3) scale: vec2f
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) tex_coords: vec2f,
    @location(1) color: vec4f,
}

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var uvs: array<vec2f, 4>;
    uvs[0] = vec2f(in.bounds.x, in.bounds.z);
    uvs[1] = vec2f(in.bounds.y, in.bounds.z);
    uvs[2] = vec2f(in.bounds.y, in.bounds.w);
    uvs[3] = vec2f(in.bounds.x, in.bounds.w);

    let matrix = mat4x4f(
        in.scale.x * uniforms.scale.x, 0.0, 0.0, 0.0,
        0.0, in.scale.y * uniforms.scale.y, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        in.char_pos.x * uniforms.scale.x + uniforms.position.x, in.char_pos.y * uniforms.scale.x + uniforms.position.y - (in.char_pos.y * uniforms.scale.x) / 2.0, uniforms.position.z, 1.0
    );
    var out: VertexOutput;
    out.position = env.matrix * matrix * vec4f(in.pos, 1.0);
    out.tex_coords = uvs[in.vertex_index];
    out.color = uniforms.color;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4f {
    return in.color * vec4f(1.0, 1.0, 1.0, textureSample(bitmap, bitmap_sampler, in.tex_coords).r);
}
