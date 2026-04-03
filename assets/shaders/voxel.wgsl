// This structure need to be kept in sync with CPU code.
struct BlockState
{
    id_raw: u32,
};

struct Enviromnent
{
    view_matrix: mat4x4<f32>,
    time: f32,
};

struct Model
{
    model_matrix: mat4x4<f32>,
}

struct VertexInput
{
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uvt: vec3<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,

    @location(0) world_position: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) normal: vec3<f32>,
    @location(3) light_vec: vec3<f32>,
    @location(4) texture_index: u32,
    @location(5) gradient_color: vec3<f32>,
};

@group(0) @binding(0) var images: texture_2d_array<f32>;
@group(0) @binding(1) var images_sampler: sampler;
@group(0) @binding(2) var<uniform> env: Enviromnent;
@group(0) @binding(3) var<uniform> model: Model;

@vertex
fn vertex_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput = VertexOutput();
    out.gradient_color = vec3<f32>(0.0, 0.0, 0.0);

    out.world_position = model.model_matrix * vec4<f32>(vertex.position, 1.0);
    out.position = env.view_matrix * out.world_position;

    out.uv = vertex.uvt.xy;
    out.normal = vertex.normal;
    out.light_vec = normalize(vec3<f32>(-1.0, -1.0, 0.0));
    out.texture_index = u32(vertex.uvt.z);

    return out;
}

fn is_grayscale(color: vec4<f32>) -> bool {
    return color.r == color.g && color.g == color.b;
}

fn is_black(color: vec3<f32>) -> bool {
    return all(color == vec3(0.0));
}

fn srgb2physical(color: vec3<f32>) -> vec3<f32> {
    let f = pow((color + 0.055) / 1.055, vec3<f32>(2.4));
    let t = color / 12.92;
    return select(f, t, color <= vec3<f32>(0.04045));
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;

    var color = textureSample(images, images_sampler, uv2, in.texture_index);
    // FIXME: since mipmaps are generated for block textures, SRGB cannot be used so we convert here.
    color = vec4(srgb2physical(color.xyz), color.a);

    if (is_grayscale(color) && !is_black(in.gradient_color)) {
        color *= vec4<f32>(in.gradient_color, 1.0);
    }

    let N = in.normal;
    let L = in.light_vec;
    let V = normalize(in.world_position.xyz);
    let R = normalize(-reflect(L, N));
    let diffuse = max(dot(N, -L), 1.0) * color.rgb;

    return vec4<f32>(diffuse, 1.0);
}
