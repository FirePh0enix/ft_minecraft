// This structure need to be kept in sync with CPU code.
struct BlockState
{
    id_raw: u32,
};

struct Enviromnent
{
    view_matrix: mat4x4<f32>,
};

struct VertexInput
{
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uvt: vec3<f32>,

    @location(3) chunk_position: vec3<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,

    @location(0) world_position: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) normal: vec3<f32>,
    @location(3) light_vec: vec3<f32>,
};

@group(0) @binding(0) var image: texture_2d<f32>;
@group(0) @binding(1) var image_sampler: sampler;
@group(0) @binding(2) var<uniform> env: Enviromnent;

@vertex
fn vertex_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput = VertexOutput();

    out.world_position = vec4<f32>(vertex.position + vertex.chunk_position, 1.0);
    out.position = env.view_matrix * out.world_position;

    out.uv = vertex.uvt.xy;
    out.normal = vertex.normal;
    out.light_vec = normalize(vec3<f32>(-1.0, -1.0, 0.0));

    return out;
}

fn is_grayscale(color: vec4<f32>) -> bool {
    return color.r == color.g && color.g == color.b;
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

    var color = textureSample(image, image_sampler, uv2);
    // color = vec4(srgb2physical(color.xyz), color.a);

    // color *= vec4<f32>(in.gradient_color, 1.0) * f32(in.has_gradient);

    // if (is_grayscale(color)) {
    //     color *= vec4<f32>(in.gradient_color, 1.0);
    // }

    const minimum_brightness = 0.3;

    let N = in.normal;
    let L = in.light_vec;
    let V = normalize(in.world_position.xyz);
    let R = normalize(-reflect(L, N));
    let diffuse = max(dot(N, -L), minimum_brightness) * color.rgb;

    return vec4<f32>(diffuse, color.a);
}
