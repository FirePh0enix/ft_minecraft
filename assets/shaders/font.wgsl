#ifdef __has_immediate
enable chromium_experimental_immediate;
#endif

struct Data
{
    color: vec4<f32>,
    position: vec3<f32>,
    scale: vec2<f32>,
}

struct Constants
{
    view_matrix: mat4x4<f32>,
}

@group(0) @binding(0) var bitmap: texture_2d<f32>;
@group(0) @binding(1) var bitmap_sampler: sampler;

@group(0) @binding(2) var<uniform> data: Data;

#ifdef __has_immediate
var<immediate> constants: Constants;
#else
@group(1) @binding(0) var<uniform> constants: Constants;
#endif

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) tex_coords: vec2<f32>,
    @location(1) color: vec4<f32>,
}

@vertex
fn vertex_main(
    @builtin(vertex_index) vertex_index: u32,
    @location(0) pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,

    @location(3) bounds: vec4<f32>,
    @location(4) char_pos: vec3<f32>,
    @location(5) scale: vec2<f32>
) -> VertexOutput
{
    var uvs: array<vec2<f32>, 4>;
    uvs[0] = vec2<f32>(bounds.x, bounds.z);
    uvs[1] = vec2<f32>(bounds.y, bounds.z);
    uvs[2] = vec2<f32>(bounds.y, bounds.w);
    uvs[3] = vec2<f32>(bounds.x, bounds.w);

    let translation_matrix = mat4x4<f32>(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        char_pos.x * data.scale.x + data.position.x, char_pos.y * data.scale.x + data.position.y, char_pos.z + data.position.z, 1.0
    );

    let scale_matrix = mat4x4<f32>(
        scale.x * data.scale.x, 0.0, 0.0, 0.0,
        0.0, scale.y * data.scale.x, 0.0, 0.0, // FIXME: For some reason `data.scale.y` is always 0.0
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );

    let model_matrix = translation_matrix * scale_matrix;

    var out: VertexOutput;
    out.position = constants.view_matrix * model_matrix * vec4<f32>(pos, 1.0);
    out.tex_coords = uvs[vertex_index];
    out.color = data.color;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32>
{
    return in.color * vec4f(1.0, 1.0, 1.0, textureSample(bitmap, bitmap_sampler, in.tex_coords).r);
}
