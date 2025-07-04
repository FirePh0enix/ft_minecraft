#ifdef __has_immediate
enable chromium_experimental_immediate;
#endif

struct Constants
{
    view_matrix: mat4x4<f32>,
    nan: f32,
}

@group(0) @binding(0) var images: texture_2d_array<f32>;
@group(0) @binding(1) var images_sampler: sampler;

#ifdef __has_immediate
var<immediate> constants: Constants;
#else
@group(1) @binding(0) var<uniform> constants: Constants;
#endif

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,

    @location(0) frag_pos: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) normal: vec3<f32>,
    @location(3) light_vec: vec3<f32>,
    @location(4) @interpolate(flat) texture_index: u32,
    @location(5) @interpolate(flat) gradient: u32,
    @location(6) @interpolate(flat) gradient_type: u32,
}

@vertex
fn vertex_main(
    @builtin(vertex_index) vertex_index: u32,
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,

    @location(3) instance_pos: vec3<f32>,
    @location(4) textures: vec3<u32>,
    @location(5) visibility_gradient: u32,
) -> VertexOutput
{
    let visibility = visibility_gradient & 0xff;
    let gradient = (visibility_gradient >> 8) & 255;
    let gradient_type = (visibility_gradient >> 16) & 255;

    var out: VertexOutput;

    if (((visibility & (1 << 1)) == 0 && vertex_index >= 0 && vertex_index < 4) ||
        ((visibility & (1 << 0)) == 0 && vertex_index >= 4 && vertex_index < 8) ||
        ((visibility & (1 << 2)) == 0 && vertex_index >= 8 && vertex_index < 12) ||
        ((visibility & (1 << 3)) == 0 && vertex_index >= 12 && vertex_index < 16) ||
        ((visibility & (1 << 4)) == 0 && vertex_index >= 16 && vertex_index < 20) ||
        ((visibility & (1 << 5)) == 0 && vertex_index >= 20 && vertex_index < 24))
    {
        out.position = vec4(constants.nan, constants.nan, constants.nan, constants.nan);
        return out;
    }

    out.gradient = gradient;
    out.gradient_type = gradient_type;

    let model_matrix = mat4x4<f32>(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        instance_pos.x, instance_pos.y, instance_pos.z, 1.0
    );

    out.position = constants.view_matrix * model_matrix * vec4<f32>(position, 1.0);

#ifndef DEPTH_PASS
    out.frag_pos = model_matrix * vec4<f32>(position, 1.0);
    out.uv = uv;
    out.normal = normal;
    out.light_vec = vec3<f32>(-1.0, -1.0, 0.0);

    let texture_indices = array<u32, 6>(textures.x & 0xFFFF, textures.x >> 16, textures.y & 0xFFFF, textures.y >> 16, textures.z & 0xFFFF, textures.z >> 16);
    out.texture_index = texture_indices[vertex_index / 4];
#endif

    return out;
}

fn is_grayscale(color: vec4<f32>) -> bool
{
    return color.r == color.g && color.g == color.b;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32>
{
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;
    
    var color = textureSample(images, images_sampler, uv2, in.texture_index);

    if (is_grayscale(color) && in.gradient_type > 0)
    {
        color *= vec4(13.0 / 255.0, 94.0 / 255.0, 21.0 / 255.0, 1.0);
    }

    let N = normalize(in.normal);
    let L = normalize(in.light_vec);
    let V = normalize(in.position.xyz);
    let R = normalize(-reflect(L, N));
    let diffuse = max(dot(N, -L), 0.1) * color.rgb;

    return vec4(diffuse, 1.0) * color;
}
