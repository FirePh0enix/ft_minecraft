struct Camera {
    view_matrix: mat4x4f,
    inv_view_matrix: mat4x4f,
    projection_matrix: mat4x4f,
}

struct WorldUniforms {
    fog_color: vec4f,
    fog_distance: f32,
    underwater: u32,
}

struct VertexInput {
    @builtin(vertex_index) vertex_index: u32,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}

@group(0) @binding(0) var albedo_texture: texture_2d<f32>;
@group(0) @binding(1) var albedo_texture_sampler: sampler;

@group(0) @binding(2) var position_texture: texture_2d<f32>;
@group(0) @binding(3) var position_texture_sampler: sampler;

@group(0) @binding(4) var normal_texture: texture_2d<f32>;
@group(0) @binding(5) var normal_texture_sampler: sampler;

@group(0) @binding(6) var ssao_texture: texture_2d<f32>;
@group(0) @binding(7) var ssao_texture_sampler: sampler;

@group(0) @binding(8) var depth_texture: texture_2d<f32>;
@group(0) @binding(9) var depth_texture_sampler: sampler;

@group(0) @binding(10) var<uniform> camera: Camera;
@group(0) @binding(11) var<uniform> world: WorldUniforms;

@vertex
fn vertex_main(in: VertexInput) -> VertexOutput {
    let u = f32((in.vertex_index << 1u) & 2u);
    let v = f32(in.vertex_index & 2u);

    var out: VertexOutput;
    out.uv = vec2f(u, 1.0 - v);
    out.position = vec4f(u * 2.0 - 1.0, v * 2.0 - 1.0, 0.0, 1.0);
    return out;
}

fn compute_volumetric_fog(world_pos: vec3f, camera_to_world_pos: vec3f) -> f32 {
    return max(min(length(world_pos - camera_to_world_pos) / world.fog_distance - 0.8, 1.0), 0.0);
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4f {
    let albedo = textureSample(albedo_texture, albedo_texture_sampler, in.uv);
    let normal = textureSample(normal_texture, normal_texture_sampler, in.uv);
    let position = textureSample(position_texture, position_texture_sampler, in.uv);
    let depth = textureSample(depth_texture, depth_texture_sampler, in.uv);

    if (albedo.a == 0.0) {
        discard;
    }

    let texel_size = 1.0 / vec2f(textureDimensions(ssao_texture));
    var ssao_value = 0.0;
    for (var x = -2; x <= 2; x++) {
       for (var y = -2; y <= 2; y++) {
           let offset = vec2f(f32(x), f32(y)) * texel_size;
           ssao_value += textureSample(ssao_texture, ssao_texture_sampler, in.uv + offset).r;
       }
    }

    let fog_factor = compute_volumetric_fog(position.xyz, (camera.view_matrix * vec4f()).xyz);
    var albedo_with_fog = mix(albedo * (ssao_value / 25.0), world.fog_color, fog_factor);
    if (world.underwater == 1) {
        albedo_with_fog = mix(albedo_with_fog, vec4f(0.0, 0.0, 1.0, 1.0), 0.5);
    }
    return albedo_with_fog;
}
