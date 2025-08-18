#ifdef __has_immediate
enable chromium_experimental_immediate;
#endif

struct Constants
{
    view_matrix: mat4x4<f32>,
    nan: f32,
}

struct Uniforms
{
    model_matrix: mat4x4<f32>,
    color: vec3<f32>,
}

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
}

#ifdef __has_immediate
var<immediate> constants: Constants;
#else
@group(1) @binding(0) var<uniform> constants: Constants;
#endif

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vertex_main(
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
) -> VertexOutput
{
    var out: VertexOutput;
    out.position = constants.view_matrix * uniforms.model_matrix * vec4<f32>(position, 1.0);
    out.color = uniforms.color;
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32>
{
    return vec4(in.color, 1.0);
}
