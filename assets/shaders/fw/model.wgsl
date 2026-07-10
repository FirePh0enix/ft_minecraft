struct Model
{
    model_matrix: mat4x4<f32>,
}

struct Camera
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

@group(0) @binding(0) var<uniform> camera: Camera;
@group(0) @binding(1) var<uniform> model: Model;
@group(0) @binding(2) var<uniform> global_model: Model;

@group(0) @binding(3) var<uniform> uvs: array<vec4<f32>, 12>;

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
    out.position = camera.view_matrix * out.world_position;
    out.normal = normal;
    out.light_vec = normalize(vec3<f32>(-1.0, -1.0, 0.0));
    
    if (vertex_index % 2 == 0) {
        out.uv = uvs[vertex_index / 2].xy;
    } else {
        out.uv = uvs[vertex_index / 2].zw;
    }

    out.frag_pos_light_space = world_env.light_view_projection * chunk.model * vec4f(in.position, 1.0);

    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(image, image_sampler, in.uv);
    return lighting(color, in.normal, in.frag_pos_light_space);
}
