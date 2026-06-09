#include "Core/StringView.hpp"

#pragma region 3D shaders

static const StringView chunk_mesh_shader_source = R"(
struct Enviromnent
{
    view_matrix: mat4x4<f32>,
};

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

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uvt: vec3<f32>,
    @location(3) chunk_position: vec3<f32>, // instance
) -> VertexOutput {
    var out: VertexOutput = VertexOutput();
    out.gradient_color = vec3<f32>(1.0, 1.0, 1.0);

    out.world_position = vec4<f32>(position + chunk_position, 1.0);
    out.position = env.view_matrix * out.world_position;

    out.uv = uvt.xy;
    out.normal = normal;
    out.light_vec = normalize(vec3<f32>(-1.0, -1.0, 0.0));
    out.texture_index = u32(uvt.z);

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

    var color = textureSample(images, images_sampler, uv2, in.texture_index);
    // FIXME: since mipmaps are generated for block textures, SRGB cannot be used so we convert here.
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
)";

static const StringView water_mesh_shader_source = R"(
struct Enviromnent
{
    view_matrix: mat4x4<f32>,
};

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
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uvt: vec3<f32>,

    @location(3) chunk_position: vec3<f32>,
) -> VertexOutput {
    var out: VertexOutput = VertexOutput();

    out.world_position = vec4<f32>(position + chunk_position, 1.0);
    out.position = env.view_matrix * out.world_position;

    out.uv = uvt.xy;
    out.normal = normal;
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
)";

static const StringView model_shader_source = R"(
struct Model
{
    model_matrix: mat4x4<f32>,
}

struct Enviromnent
{
    view_matrix: mat4x4<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) world_position: vec4<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) light_vec: vec3<f32>,
    @location(3) uv: vec2<f32>,
}

@group(0) @binding(0) var<uniform> env: Enviromnent;
@group(0) @binding(1) var<uniform> model: Model;
@group(0) @binding(2) var<uniform> global_model: Model;

@group(0) @binding(3) var<uniform> uvs: array<vec4<f32>, 12>;

// TODO: to optimize bind group reuse, this should be group 1, let's do that in the new renderer
@group(0) @binding(4) var image: texture_2d<f32>;
@group(0) @binding(5) var image_sampler: sampler;

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    
    @builtin(vertex_index) vertex_index: u32,
) -> VertexOutput {
    var out: VertexOutput;
    out.world_position = global_model.model_matrix * model.model_matrix * vec4<f32>(position, 1.0);
    out.position = env.view_matrix * out.world_position;
    out.normal = normal;
    out.light_vec = normalize(vec3<f32>(-1.0, -1.0, 0.0));
    
    if (vertex_index % 2 == 0) {
        out.uv = uvs[vertex_index / 2].xy;
    } else {
        out.uv = uvs[vertex_index / 2].zw;
    }

    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    const minimum_brightness = 0.1;

    let color = textureSample(image, image_sampler, in.uv);

    let N = in.normal;
    let L = in.light_vec;
    let V = normalize(in.world_position.xyz);
    let R = normalize(-reflect(L, N));
    let diffuse = max(dot(N, -L), minimum_brightness) * color.rgb;

    return vec4<f32>(diffuse, 1.0);
}
)";

static const StringView block_preview_shader_source = R"(
struct Model
{
    model_matrix: mat4x4<f32>,
    textures: vec3<u32>,
}

struct VertexOutput
{
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
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,

    @builtin(vertex_index) index: u32,
) -> VertexOutput {
    var out: VertexOutput;
    out.world_position = vec4<f32>(position, 1.0);
    out.position = model.model_matrix * out.world_position;
    out.uv = uv;

    var offset: u32 = 0;
    if (index % 8 > 3) {
        offset = 16;
    }

    out.normal = normal;
    out.light_vec = normalize(vec3<f32>(-1.0, 0.0, 0.0));
    out.texture = (model.textures[index / 8] >> offset) & 0xFFFF;

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
)";

static const StringView item_block_shader_source = R"(
struct Enviromnent
{
    view_matrix: mat4x4<f32>,
};

struct Model
{
    model_matrix: mat4x4<f32>,
    textures: vec3<u32>,
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
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,

    @builtin(vertex_index) index: u32,
) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.view_matrix * model.model_matrix * vec4<f32>(position, 1.0);
    out.uv = uv;

    var offset: u32 = 0;
    if (index % 8 > 3) {
        offset = 16;
    }

    out.texture = (model.textures[index / 8] >> offset) & 0xFFFF;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;
    var color = textureSample(images, images_sampler, uv2, in.texture);
    // color = vec4(srgb2physical(color.xyz), color.a);
    return color;
}
)";

static const StringView simple_shape_source = R"(
struct Enviromnent
{
    view_matrix: mat4x4<f32>,
};

struct Model
{
    model_matrix: mat4x4<f32>,
    color: vec4<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
}

@group(0) @binding(0) var<uniform> env: Enviromnent;
@group(0) @binding(1) var<uniform> model: Model;

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.view_matrix * model.model_matrix * vec4<f32>(position, 1.0);
    out.color = model.color;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return in.color;
}
)";

static const StringView sky_shader_source = R"(
@vertex
fn vertex_main() {
}

@fragment
fn fragment_main() -> @location(0) vec4<f32> {
}
)";

#pragma endregion

#pragma region UI shaders

static const StringView color_rect_shader_source = R"(
struct Env
{
    matrix: mat4x4<f32>,
}

struct Uniforms
{
    matrix: mat4x4<f32>,
    color: vec4<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
}

@group(0) @binding(0) var<uniform> env: Env;
@group(0) @binding(1) var<uniform> uniforms: Uniforms;

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * uniforms.matrix * vec4(position, 1.0);
    out.color = uniforms.color;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return in.color;
}
)";

static const StringView texture_rect_shader_source = R"(
struct Env
{
    matrix: mat4x4<f32>,
}

struct Uniforms
{
    matrix: mat4x4<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) uv: vec2<f32>,
}

@group(0) @binding(0) var<uniform> env: Env;
@group(0) @binding(1) var<uniform> uniforms: Uniforms;
@group(0) @binding(2) var image: texture_2d<f32>;
@group(0) @binding(3) var image_sampler: sampler;

// fn srgb2physical(color: vec3<f32>) -> vec3<f32> {
//     let f = pow((color + 0.055) / 1.055, vec3<f32>(2.4));
//     let t = color / 12.92;
//     return select(f, t, color <= vec3<f32>(0.04045));
// }

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) uv: vec2<f32>,
) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * uniforms.matrix * vec4(position, 1.0);
    out.uv = uv;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;
    var color = textureSample(image, image_sampler, uv2);
    // color = vec4(srgb2physical(color.xyz), color.a);
    return color;
}
)";

#pragma endregion
