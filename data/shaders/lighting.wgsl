fn shadowMap(normal: vec3<f32>, frag_pos_light_space: vec4<f32>) -> f32 {
    // The light projection is orthographic (w = 1), so no perspective divide
    // is needed. Flip Y when mapping clip coordinates to texture coordinates.
    let uv = frag_pos_light_space.xy * vec2(0.5, -0.5) + vec2(0.5);

    // Outside the light's orthographic volume there is no valid shadow-map
    // sample. Treat it as lit instead of clamping to a dark border texel.
    if (frag_pos_light_space.z < 0.0 || frag_pos_light_space.z > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return 1.0; 
    }

    // Smooth dynamic slope-scaled bias
    let cos_theta = clamp(dot(normal, -world_env.light_dir), 0.0, 1.0);
    let bias = max(0.003 * (1.0 - cos_theta), 0.0005);

    // The comparison sampler uses linear filtering, so one sample performs
    // hardware-filtered 2x2 PCF. A software 3x3 loop would multiply this into
    // 36 depth comparisons for every terrain pixel.
    let depth_to_compare = frag_pos_light_space.z - bias;
    return textureSampleCompareLevel(shadowmap, shadowmap_sampler, uv, depth_to_compare);
}

fn lighting(color: vec4<f32>, normal: vec3<f32>, shadow_pos: vec4<f32>) -> vec4<f32> {
    let diffuse_factor = clamp(dot(world_env.light_dir, normal), 0.0, 1.0);
    
    // visibility is in [0, 1]: diffuse <= 0.1 plus ambient 0.2 never exceeds
    // the 0.3 lighting floor, so sampling cannot change the result. Keep this
    // threshold in sync with the ambient term and clamp below.
    // textureSampleCompareLevel needs no screen-space derivatives, making it
    // valid even when neighboring fragments take different branches.
    if (diffuse_factor <= 0.1) {
        return vec4<f32>(color.rgb * 0.3, color.a);
    }
    let visibility = shadowMap(normal, shadow_pos); 
    
    let ambient_light = 0.2;
    let combined_light = clamp(visibility * diffuse_factor + ambient_light, 0.3, 1.0);
    
    return vec4<f32>(color.rgb * combined_light, color.a);
}
