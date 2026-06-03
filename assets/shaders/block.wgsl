struct Enviromnent
{
    view_matrix: mat4x4<f32>,
};

struct Model
{
    model_matrix: mat4x4<f32>,
    textures: vec3<u32>,
}

struct VertexInput
{
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,

    @builtin(vertex_index) index: u32,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(3) texture: u32,
}

@group(0) @binding(0) var<uniform> env: Enviromnent;
@group(0) @binding(1) var<uniform> model: Model;
@group(0) @binding(2) var images: texture_2d_array<f32>;
@group(0) @binding(3) var images_sampler: sampler;

fn srgb2physical(color: vec3<f32>) -> vec3<f32> {
    let f = pow((color + 0.055) / 1.055, vec3<f32>(2.4));
    let t = color / 12.92;
    return select(f, t, color <= vec3<f32>(0.04045));
}

@vertex
fn vertex_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.view_matrix * model.model_matrix * vec4<f32>(vertex.position, 1.0);
    out.uv = vertex.uv;

    var offset: u32 = 0;
    if (vertex.index % 8 > 3) {
        offset = 16;
    }

    out.texture = (model.textures[vertex.index / 8] >> offset) & 0xFFFF;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;
    var color = textureSample(images, images_sampler, uv2, in.texture);
    color = vec4(srgb2physical(color.xyz), color.a);
    return color;
}
