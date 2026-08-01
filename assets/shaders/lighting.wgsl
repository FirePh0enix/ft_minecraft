fn shadowMap(normal: vec3<f32>, frag_pos_light_space: vec4<f32>) -> f32 {
    let uv = frag_pos_light_space.xy * vec2(0.5, -0.5) + vec2(0.5);

    if (frag_pos_light_space.z > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return 1.0; 
    }

    let texel_size = vec2<f32>(1.0) / vec2<f32>(textureDimensions(shadowmap));
    
    // Smooth dynamic slope-scaled bias
    let cos_theta = clamp(dot(normal, -world_env.light_dir), 0.0, 1.0);
    let bias = max(0.003 * (1.0 - cos_theta), 0.0005);
    
    // We compare our fragment depth minus the safety bias padding
    let depth_to_compare = frag_pos_light_space.z - bias;
    var visibility = 0.0;

    // Hardware comparison sampling across the 3x3 grid
    for (var x = -1; x <= 1; x++) {
        for (var y = -1; y <= 1; y++) {
            let offset = vec2<f32>(f32(x), f32(y));
            
            // textureSampleCompare automatically checks depth and returns a 0.0 to 1.0 range
            visibility += textureSampleCompare(
                shadowmap, 
                shadowmap_sampler, 
                uv + texel_size * offset, 
                depth_to_compare
            );
        }
    }
    
    // Return average light intensity (1.0 = fully lit, 0.0 = fully in shadow)
    return visibility / 9.0;
}

fn lighting(color: vec4<f32>, normal: vec3<f32>, shadow_pos: vec4<f32>) -> vec4<f32> {
    let diffuse_factor = clamp(dot(world_env.light_dir, normal), 0.0, 1.0);
    
    // Adjusted: shadowMap now returns visibility directly
    let visibility = shadowMap(normal, shadow_pos); 
    
    let ambient_light = 0.2;
    let combined_light = clamp(visibility * diffuse_factor + ambient_light, 0.3, 1.0);
    
    return vec4<f32>(color.rgb * combined_light, color.a);
}