struct Model
{
    model_matrix: mat4x4<f32>,
}

struct Enviromnent
{
    view_matrix: mat4x4<f32>,
    time: f32,
}

struct VertexInput
{
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    
    @builtin(vertex_index) vertex_index: u32,
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
fn vertex_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.world_position = global_model.model_matrix * model.model_matrix * vec4<f32>(vertex.position, 1.0);
    out.position = env.view_matrix * out.world_position;
    out.normal = vertex.normal;
    out.light_vec = normalize(vec3<f32>(-1.0, -1.0, 0.0));
    
    if (vertex.vertex_index % 2 == 0) {
        out.uv = uvs[vertex.vertex_index / 2].xy;
    } else {
        out.uv = uvs[vertex.vertex_index / 2].zw;
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
