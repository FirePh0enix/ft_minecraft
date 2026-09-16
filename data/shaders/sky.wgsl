struct SkyUniforms {
    inv_view_proj: mat4x4f,
    color: vec4<f32>,
}

struct WorldEnv {
    light_view_projection: mat4x4f,
    light_dir: vec3f, // Points TOWARDS the sun
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) clip_position: vec2f,
}

@group(0) @binding(0) var<uniform> sky: SkyUniforms;
@group(0) @binding(1) var<uniform> world_env: WorldEnv;

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    let u = f32((vertex_index << 1u) & 2u);
    let v = f32(vertex_index & 2u);

    let x = -1.0 + f32((vertex_index & 1u) << 2u);
    let y = -1.0 + f32((vertex_index & 2u) << 1u);

    var out: VertexOutput;
    out.uv = vec2f(u, v);
    out.clip_position = vec2f(x, y);
    out.position = vec4f(x, y, 0.0, 1.0);
    return out;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4f {
    // 1. Calculate the view direction ray
    let far_target = sky.inv_view_proj * vec4f(in.clip_position, 1.0, 1.0);
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

    // 5. Procedural Sun (only renders if sun is above the horizon)
    if (sun_height > -0.05) {
        let sun_direction = normalize(world_env.light_dir); 
        let cos_angle = dot(ray_dir, sun_direction);

        // Make the sun slightly larger and warmer near the horizon
        let sun_size = mix(0.992, 0.995, day_factor); 
        let sun_blur = 0.003;
        
        // Sun changes color from bright white at noon to deep gold at sunset
        let sun_color = mix(vec3f(1.0, 0.5, 0.2), vec3f(1.0, 0.98, 0.9), day_factor);

        let sun_mask = smoothstep(sun_size - sun_blur, sun_size, cos_angle);
        final_color = mix(final_color, sun_color, sun_mask);
    }

    return vec4f(final_color, 1.0);
}
