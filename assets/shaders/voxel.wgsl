#ifdef __has_immediate
enable chromium_experimental_immediate;
#endif

struct Constants
{
    view_matrix: mat4x4<f32>,
    nan: f32,
}

const plains_grass_color = vec3(63.0 / 255.0, 155.0 / 255.0, 11.0 / 255.0);

const grass_colors: array<vec3<f32>, 14> = array(
    plains_grass_color, // FrozenRiver
    plains_grass_color, // River
    plains_grass_color, // StonyShore
    plains_grass_color, // StonyPeaks
    plains_grass_color, // JaggedPeaks
    plains_grass_color, // FrozenPeaks
    plains_grass_color, // Beach
    plains_grass_color, // Desert
    plains_grass_color, // SnowyPlains
    plains_grass_color, // Taiga
    plains_grass_color, // Plains
    plains_grass_color, // Savanna
    plains_grass_color, // FrozenOcean
    plains_grass_color, // Ocean
);

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
    @location(5) gradient_color: vec3<f32>,
}

@vertex
fn vertex_main(
    @builtin(vertex_index) vertex_index: u32,
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,

    @location(3) instance_pos: vec3<f32>,
    @location(4) textures: vec3<u32>,
    @location(5) visibility_biome_gradient: u32,
) -> VertexOutput
{
    let visibility = visibility_biome_gradient & 0xff;
    let biome = (visibility_biome_gradient >> 8) & 0xff;
    let gradient_type = (visibility_biome_gradient >> 16) & 0xff;

    var out: VertexOutput;
    out.gradient_color = vec3(0.0, 0.0, 0.0);

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

    let model_matrix = mat4x4<f32>(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        instance_pos.x, instance_pos.y, instance_pos.z, 1.0
    );

    out.position = constants.view_matrix * model_matrix * vec4<f32>(position, 1.0);

#ifndef DEPTH_PASS
    if (gradient_type == 0) // None
    {
        out.gradient_color = vec3(0.0, 0.0, 0.0);
    }
    else if (gradient_type == 1) // Grass
    {
        out.gradient_color = grass_colors[biome];
    }
    else if (gradient_type == 2) // Water
    {
    }

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

fn is_black(color: vec3<f32>) -> bool
{
    return color.r == 0.0 && color.g == 0.0 && color.b == 0.0;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32>
{
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;
    
    var color = textureSample(images, images_sampler, uv2, in.texture_index);

    if (is_grayscale(color) && !is_black(in.gradient_color))
    {
        color *= vec4(in.gradient_color, 1.0);
    }

    let N = normalize(in.normal);
    let L = normalize(in.light_vec);
    let V = normalize(in.position.xyz);
    let R = normalize(-reflect(L, N));
    let diffuse = max(dot(N, -L), 0.1) * color.rgb;

    return vec4(diffuse, 1.0) * color;
}
