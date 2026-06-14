struct Env {
    matrix: mat4x4f,
}

struct Camera {
    view_matrix: mat4x4f,
    projection_matrix: mat4x4f,
}

struct Uniforms {
    samples: array<vec4f, 64>,
}

struct Model {
    matrix: mat4x4f,
    time: f32,
}

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}

@group(0) @binding(0) var<uniform> env: Env;
@group(0) @binding(1) var<uniform> uniforms: Uniforms;
@group(0) @binding(2) var<uniform> model: Model;
@group(0) @binding(3) var<uniform> camera: Camera;

@group(0) @binding(4) var position_buffer: texture_2d<f32>;
@group(0) @binding(5) var position_buffer_sampler: sampler;

@group(0) @binding(6) var normal_buffer: texture_2d<f32>;
@group(0) @binding(7) var normal_buffer_sampler: sampler;

@group(0) @binding(8) var noise_texture: texture_2d<f32>;
@group(0) @binding(9) var noise_texture_sampler: sampler;

const noise_scale = vec2<f32>(1280.0 / 4.0, 720.0 / 4.0);

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * model.matrix * vec4f(in.position, 1.0);
    out.uv = in.uv;
    return out;
}

const kernel_size = 64;

@fragment
fn fragment_main(vertex: VertexOutput) -> @location(0) f32 {
    var uv2 = vertex.uv;
    uv2.y = 1.0 - uv2.y;

    let frag_pos = textureSample(position_buffer, position_buffer_sampler, uv2).xyz;
    let normal = textureSample(normal_buffer, normal_buffer_sampler, uv2).xyz;

    let tex_dim = textureDimensions(position_buffer);
    let noise_dim = textureDimensions(noise_texture);
    let noise_uv = vec2f(f32(tex_dim.x) / f32(noise_dim.x), f32(tex_dim.y) / f32(noise_dim.y)) * uv2;
    let random_vec = textureSample(noise_texture, noise_texture_sampler, noise_uv).xyz * 2.0 - 1.0;

    let tangent = normalize(random_vec - normal * dot(random_vec, normal));
    let bitangent = cross(tangent, normal);
    let tbn = mat3x3f(tangent, bitangent, normal);

    var occlusion = 0.0;
    let bias = 0.025;
    let radius = 0.3;
    
    for (var i = 0; i < kernel_size; i++) {
        var sample_pos = tbn * uniforms.samples[i].xyz;
        sample_pos = frag_pos + sample_pos * radius;

        var offset = vec4(sample_pos, 1.0);
        offset = camera.projection_matrix * offset;

        let ndc = offset.xyz / offset.w;
        let sample_uv = vec2f(ndc.x * 0.5 + 0.5, 1.0 - (ndc.y * 0.5 + 0.5));

        let sample_depth = textureSample(position_buffer, position_buffer_sampler, sample_uv).w;

        let range_check = smoothstep(0.0, 1.0, radius / abs(frag_pos.z - sample_depth));
        occlusion += select(0.0, 1.0, sample_depth >= sample_pos.z + bias) * range_check;
    }

    return 1.0 - (occlusion / f32(kernel_size));
}
