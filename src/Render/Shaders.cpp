#include "Core/StringView.hpp"

static const StringView chunk_mesh_shader_source = R"(
struct Environment
{
    view_matrix: mat4x4<f32>,
    sun_direction: vec3<f32>,
    sun_color: vec4<f32>,
};

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,

    @location(0) world_position: vec4<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) normal: vec3<f32>,
    @location(3) light_vec: vec3<f32>,
    @location(4) light_color: vec4<f32>,
    @location(5) texture_index: u32,
    @location(6) gradient_color: vec3<f32>,
};

fn mix(a: vec4<f32>, b: vec4<f32>, t: f32) -> vec4<f32> {
    return a * (1.0 - t) + b * t;
}

@group(0) @binding(0) var images: texture_2d_array<f32>;
@group(0) @binding(1) var images_sampler: sampler;
@group(0) @binding(2) var<uniform> env: Environment;

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
    out.light_vec = env.sun_direction;
    out.light_color = env.sun_color;
    out.texture_index = u32(uvt.z);

    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;

    var color = textureSample(images, images_sampler, uv2, in.texture_index);

    const minimum_brightness = 0.3;

    let N = in.normal;
    let L = in.light_vec;
    let V = normalize(in.world_position.xyz);
    let R = normalize(-reflect(L, N));
    let diffuse = mix(max(dot(N, -L), minimum_brightness) * color, max(dot(N, -L), minimum_brightness) * in.light_color, in.light_color.a);

    return diffuse;
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

static const StringView surface_shader_source = R"(
struct Env
{
    matrix: mat4x4<f32>,
}

struct Uniforms
{
    model_matrix: mat4x4<f32>,
    time: f32,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<uniform> env: Env;
@group(0) @binding(2) var render_target: texture_2d<f32>;
@group(0) @binding(3) var render_target_sampler: sampler;

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) uv: vec2<f32>,
) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * uniforms.model_matrix * vec4(position, 1.0);
    out.uv = uv;
    return out;
}

@fragment
fn fragment_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    var uv2 = vertex.uv;
    uv2.y = 1.0 - uv2.y;
    return textureSample(render_target, render_target_sampler, uv2);
}
)";

static const StringView sky_shader_source = R"(
struct Env
{
    matrix: mat4x4<f32>,
}

struct Uniforms
{
    model_matrix: mat4x4<f32>,
    time: f32,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<uniform> env: Env;

const night = vec3<f32>(2.0 / 255.0, 7.0 / 255.0, 26.0 / 255.0);
const daylight = vec3<f32>(135.0 / 255.0, 206.0 / 255.0, 235.0 / 255.0);
const sunset = vec3<f32>(247.0 / 255.0, 152.0 / 255.0, 2.0 / 255.0);

fn lerp(a: vec3<f32>, b: vec3<f32>, t: f32) -> vec3<f32> {
    return vec3<f32>(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t
    );
}

fn seven_color_gradient(
    c0: vec3<f32>, c1: vec3<f32>, c2: vec3<f32>, c3: vec3<f32>, c4: vec3<f32>, c5: vec3<f32>, c6: vec3<f32>,
    t: f32
) -> vec3<f32> {
    if (t <= 0.0) { return c0; }
    if (t >= 1.0) { return c6; }

    let segments: f32 = 6.0;
    let seg_len: f32 = 1.0 / segments;
    let s: f32 = floor(t / seg_len);
    let si: i32 = i32(s);
    let local_t: f32 = (t - s * seg_len) / seg_len;

    if (si == 0) {
        return lerp(c0, c1, local_t);
    } else if (si == 1) {
        return lerp(c1, c2, local_t);
    } else if (si == 2) {
        return lerp(c2, c3, local_t);
    } else if (si == 3) {
        return lerp(c3, c4, local_t);
    } else if (si == 4) {
        return lerp(c4, c5, local_t);
    } else {
        // si == 5
        return lerp(c5, c6, local_t);
    }
}

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * uniforms.model_matrix * vec4(position, 1.0);
    out.color = seven_color_gradient(night, sunset, daylight, daylight, daylight, sunset, night, uniforms.time);
    return out;
}

@fragment
fn fragment_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    return vec4(vertex.color, 1.0);
}
)";

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

static const StringView underwater_shader_source = R"(
struct Env
{
    matrix: mat4x4<f32>,
}

struct Uniforms
{
    model_matrix: mat4x4<f32>,
    time: f32,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

fn mix(a: vec4<f32>, b: vec4<f32>, t: f32) -> vec4<f32> {
    return a * (1.0 - t) + b * t;
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<uniform> env: Env;
@group(0) @binding(2) var render_target: texture_2d<f32>;
@group(0) @binding(3) var render_target_sampler: sampler;

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) uv: vec2<f32>,
) -> VertexOutput {
    var out: VertexOutput;
    out.position = env.matrix * uniforms.model_matrix * vec4(position, 1.0);
    out.uv = uv;
    return out;
}

@fragment
fn fragment_main(vertex: VertexOutput) -> @location(0) vec4<f32> {
    var uv2 = vertex.uv;
    uv2.y = 1.0 - uv2.y;
    return mix(textureSample(render_target, render_target_sampler, uv2), vec4(0.0, 0.0, 1.0, 1.0), 0.5);
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
