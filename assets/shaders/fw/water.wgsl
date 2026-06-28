struct Chunk {
    model: mat4x4f,
}

struct Camera {
    view_projection: mat4x4f,
}

struct WorldEnv {
    light_view_projection: mat4x4f,
    light_dir: vec3f,
}

@group(0) @binding(0) var<uniform> chunk: Chunk;
@group(0) @binding(1) var<uniform> camera: Camera;
@group(0) @binding(2) var<uniform> world_env: WorldEnv;

@group(0) @binding(3) var image: texture_2d<f32>;
@group(0) @binding(4) var image_sampler: sampler;

@group(0) @binding(5) var shadowmap: texture_depth_2d;
@group(0) @binding(6) var shadowmap_sampler: sampler;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
}

struct VertexOutput {
    @builtin(position) clip_position: vec4f,
    @location(0) uv: vec2f,
    @location(1) inv_normal: vec3f,
    @location(2) normal: vec3f,
    @location(3) frag_pos_light_space: vec4f,
}

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.uv = vec2f(in.uv.x, 1.0 - in.uv.y);
    out.normal = in.normal;

    out.clip_position = camera.view_projection * chunk.model * vec4f(in.position, 1.0);
    out.frag_pos_light_space = world_env.light_view_projection * chunk.model * vec4f(in.position, 1.0);

    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f,
    @location(1) debug: vec4f,
}

@fragment
fn fragment_main(in: VertexOutput) -> FragmentOutput {
    var out: FragmentOutput;
    let color = textureSample(image, image_sampler, in.uv);

    let ambient = max(dot(world_env.light_dir, in.normal), 0.2);

    if (in.frag_pos_light_space.z > 1) {
        out.color = color * max(ambient, 0.3);
        return out;
    }

    let uv = in.frag_pos_light_space.xy * vec2(0.5, -0.5) + vec2(0.5);

    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
        out.color = color * max(ambient, 0.3);
        return out;
    }
    
    var visibility = 1.0;
    if (textureSample(shadowmap, shadowmap_sampler, uv) < in.frag_pos_light_space.z - 0.005) {
        visibility = 0.0;
    }

    out.color = color * max(visibility * ambient, 0.3);

    return out;
}
