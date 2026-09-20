struct PostProcess {
    inverse_camera_proj: mat4x4f,
    camera_proj: mat4x4f,
    inv_view_proj: mat4x4f,
    fog_color: vec4f,
    fog_distance: f32,
    near: f32,
    far: f32,
    underwater: u32,
}

struct WorldEnv {
    light_view_projection: mat4x4f,
    light_dir: vec3f, // Points TOWARDS the sun
}

@group(0) @binding(0) var<uniform> uniforms: PostProcess;
@group(0) @binding(2) var<uniform> world_env: WorldEnv;

@group(0) @binding(3) var surface: texture_2d<f32>;
@group(0) @binding(4) var surface_sampler: sampler;

@group(0) @binding(5) var depth: texture_depth_2d;
@group(0) @binding(6) var depth_sampler: sampler;
@group(0) @binding(7) var ao: texture_2d<f32>;
@group(0) @binding(8) var ao_sampler: sampler;

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) clip_position: vec2f,
}

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    let u = f32((vertex_index << 1u) & 2u);
    let v = f32(vertex_index & 2u);

    let x = -1.0 + f32((vertex_index & 1u) << 2u);
    let y = -1.0 + f32((vertex_index & 2u) << 1u);

    var out: VertexOutput;
    out.uv = vec2f(u, 1.0 - v);
    out.clip_position = vec2f(x, y);
    out.position = vec4f(u * 2.0 - 1.0, v * 2.0 - 1.0, 0.0, 1.0);
    return out;
}

fn linearize_depth(d: f32) -> f32 {
    // Convert perspective depth [0, 1] to positive camera-space distance.
    // This matches -position.z stored in the AO texture's green channel.
    return uniforms.near * uniforms.far / (uniforms.far + d * (uniforms.near - uniforms.far));
}

fn simpleFog(d: f32) -> f32 {
    return max(min(length(d) / uniforms.fog_distance - 0.8, 1.0), 0.0);
}

fn get_fog_color(clip_position: vec2f) -> vec3f {
    // 1. Calculate the view direction ray
    let far_target = uniforms.inv_view_proj * vec4f(clip_position, 1.0, 1.0);
    let ray_dir = normalize(far_target.xyz / far_target.w);

    // 2. Define Time-of-Day Sky Profiles
    // Midday Sky Colors
    let day_horizon = vec3f(0.7, 0.85, 0.95);  // Light sky blue
    let day_zenith  = vec3f(0.1, 0.4, 0.75);   // Deep clear blue
    
    // Sunset / Sunrise Sky Colors
    let sunset_horizon = vec3f(0.95, 0.4, 0.15); // Vibrant orange/red
    let sunset_zenith  = vec3f(0.15, 0.15, 0.35); // Dark purple/blue

    // Night Sky Colors
    let night_horizon = vec3f(0.02, 0.04, 0.1);  // Very dark blue
    let night_zenith  = vec3f(0.005, 0.01, 0.03); // Near black

    // 3. Calculate dynamic factor based on sun height (light_dir.y)
    let sun_height = normalize(world_env.light_dir).y;
    
    // Smooth transitions between night, sunset, and full day
    // sun_height > 0.0 means sun is above horizon
    let day_factor = smoothstep(0.0, 0.3, sun_height); 
    // sun_height < 0.0 means sun is below horizon
    let night_factor = smoothstep(0.0, -0.2, sun_height);

    // Interpolate horizon and zenith colors based on time of day
    var active_horizon = mix(sunset_horizon, day_horizon, day_factor);
    active_horizon = mix(active_horizon, night_horizon, night_factor);

    var active_zenith = mix(sunset_zenith, day_zenith, day_factor);
    active_zenith = mix(active_zenith, night_zenith, night_factor);

    // 4. Base Sky Gradient
    let h = max(ray_dir.y, 0.0);
    let t = pow(h, 2.0);
    var final_color = mix(active_horizon, active_zenith, t);

    return final_color;
}

// Filter and upsample together: each fetch supplies visibility AND its
// source depth, avoiding a separate full-screen blur and extra depth reads.
fn filtered_visibility(pixel: vec2i, center_depth: f32) -> f32 {
    let size = vec2i(textureDimensions(ao));
    let full_size = vec2i(textureDimensions(depth));
    let base = pixel / 2;
    var visibility = 0.0;
    var total_weight = 0.0;
    for (var y = -1; y <= 1; y += 1) {
        for (var x = -1; x <= 1; x += 1) {
            let p = clamp(base + vec2i(x, y), vec2i(0), size - vec2i(1));
            let sample = textureLoad(ao, p, 0).rg;
            // Match the representative texel selected by ssao.wgsl, rather
            // than assuming the AO sample is centered between four pixels.
            let source_pixel = min(p * 2 + vec2i(1), full_size - vec2i(1));
            let delta = (vec2f(source_pixel) - vec2f(pixel)) * 0.5;
            let spatial_weight = 1.0 / (1.0 + dot(delta, delta) * 2.0);
            // Bilateral filtering weights both image distance and depth.
            // The thresholds are in world units: reject unrelated surfaces
            // across block silhouettes instead of blurring AO onto them.
            let depth_delta = abs(sample.g - center_depth);
            let depth_weight = 1.0 - smoothstep(0.05, 0.3, depth_delta);
            // A zero stored depth denotes sky and must never contribute.
            let weight = spatial_weight * depth_weight * select(0.0, 1.0, sample.g > 0.0);
            visibility += sample.r * weight;
            total_weight += weight;
        }
    }
    // Thin geometry missing from the half-resolution buffer stays unoccluded
    // instead of borrowing a shadow from an unrelated foreground surface.
    if (total_weight < 0.0001) {
        return 1.0;
    }
    return visibility / total_weight;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(surface, surface_sampler, in.uv);
    let pixel = vec2i(in.position.xy);
    let raw_depth = textureLoad(depth, pixel, 0);
    if (raw_depth >= 1.0) {
        return color;
    }

    let ao_factor = filtered_visibility(pixel, linearize_depth(raw_depth));
    let lit_color = vec4f(color.rgb * ao_factor, color.a);
    // Apply AO before fog so distant contact shadows fade with the scene.
    let fog_factor = simpleFog(linearize_depth(raw_depth));
    var final_color = mix(lit_color, vec4f(get_fog_color(in.clip_position), 1.0), fog_factor);
    if (uniforms.underwater == 1) {
        final_color = mix(final_color, vec4f(0.0, 0.0, 1.0, 1.0), 0.5);
    }
    return final_color;
}
