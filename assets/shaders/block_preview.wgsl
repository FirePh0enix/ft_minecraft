struct Model {
    model_matrix: mat4x4<f32>,
    textures: vec3<u32>,
}

struct VertexInput {
    @builtin(vertex_index) index: u32,
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) normal: vec3<f32>,
    @location(3) texture: u32,
    @location(4) light_vec: vec3<f32>,
    @location(5) world_position: vec4<f32>,
}

@group(0) @binding(0) var<uniform> model: Model;
@group(0) @binding(1) var images: texture_2d_array<f32>;
@group(0) @binding(2) var images_sampler: sampler;

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.world_position = vec4<f32>(in.position, 1.0);
    out.position = model.model_matrix * out.world_position;
    out.uv = in.uv;

    var offset: u32 = 0;
    if (in.index % 8 > 3) {
        offset = 16;
    }

    out.normal = in.normal;
    out.light_vec = normalize(vec3<f32>(-1.0, 0.0, 0.0));
    out.texture = (model.textures[in.index / 8] >> offset) & 0xFFFF;

    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;
    var color = textureSample(images, images_sampler, uv2, in.texture);

    const minimum_brightness = 0.1;

    let N = in.normal;
    let L = in.light_vec;
    let V = normalize(in.world_position.xyz);
    let R = normalize(-reflect(L, N));
    let diffuse = max(dot(N, -L), minimum_brightness) * color.rgb;

    return color;
}
