fn shadowMap(normal: vec3<f32>, frag_pos_light_space: vec4<f32>) -> f32 {
    let uv = frag_pos_light_space.xy * vec2(0.5, -0.5) + vec2(0.5);

    if (frag_pos_light_space.z > 1 || uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
	return 1.0;
    }

    let texel_size = vec2<f32>(1.0) / vec2<f32>(textureDimensions(shadowmap));
    // TODO: correct the bias formula should fix shadow issues.
    //let bias = max(0.05 * (1.0 - dot(-normal, normalize(world_env.light_dir))), 0.005);
    let bias = 0.000;
    var visibility = 0.0;

    for (var x = -1; x < 1; x++) {
	for (var y = -1; y < 1; y++) {
	    let offset = vec2(f32(x), f32(y));
	    if (textureSample(shadowmap, shadowmap_sampler, uv + texel_size * offset) < frag_pos_light_space.z - bias) {
		visibility += 1.0;
	    }
	}
    }
    
    return visibility / 9.0;
}

fn lighting(color: vec4<f32>, normal: vec3<f32>, shadow_pos: vec4<f32>) -> vec4<f32> {
    let ambient = max(dot(world_env.light_dir, normal), 0.2);
    let visibility = 1.0 - shadowMap(normal, shadow_pos);
    return vec4(color.rgb * max(visibility * ambient, 0.3), color.a);
}
