struct PostProcess {
    inverse_camera_proj: mat4x4f,
    camera_proj: mat4x4f, // Added: Required to project samples back to screen space
    fog_color: vec4<f32>,
    fog_distance: f32,
    near: f32,
    far: f32,
    underwater: u32,
}

struct SSAO {
    samples: array<vec4f, 64>,
}

@group(0) @binding(0) var<uniform> uniforms: PostProcess;
@group(0) @binding(1) var<uniform> ssao: SSAO;

@group(0) @binding(2) var surface: texture_2d<f32>;
@group(0) @binding(3) var surface_sampler: sampler;

@group(0) @binding(4) var depth: texture_depth_2d;
@group(0) @binding(5) var depth_sampler: sampler;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
}

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    let u = f32((vertex_index << 1u) & 2u);
    let v = f32(vertex_index & 2u);

    var out: VertexOutput;
    out.uv = vec2f(u, 1.0 - v);
    out.position = vec4f(u * 2.0 - 1.0, v * 2.0 - 1.0, 0.0, 1.0);
    return out;
}

fn linearize_depth(d: f32) -> f32 {
    return uniforms.near * uniforms.far / (uniforms.far + d * (uniforms.near - uniforms.far));
}

fn simpleFog(d: f32) -> f32 {
    return max(min(length(d) / uniforms.fog_distance - 0.8, 1.0), 0.0);
}

// Helper function to get view-space position from UV and depth
fn get_view_pos(uv: vec2f, raw_depth: f32) -> vec3f {
    // Convert UV and depth to Normalized Device Coordinates (NDC)
    // WebGPU depth is [0.0, 1.0]
    let ndc = vec4f(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0, raw_depth, 1.0);
    let view_pos_clip = uniforms.inverse_camera_proj * ndc;
    return view_pos_clip.xyz / view_pos_clip.w;
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = textureSample(surface, surface_sampler, in.uv);
    let raw_depth = textureSample(depth, depth_sampler, in.uv);
    
    // Skip SSAO and Fog for skybox background
    if (raw_depth >= 1.0) {
        return color;
    }

    // 1. RECONSTRUCT VIEW-SPACE GEOMETRY
    let view_pos = get_view_pos(in.uv, raw_depth);
    
    // Estimate a view-space surface normal using screen derivatives (partial derivatives)
    // This removes the need for a separate G-buffer normal texture pass
    let normal = normalize(cross(dpdx(view_pos), dpdy(view_pos)));

    // 2. COMPUTE SSAO OCCLUSION FACTOR
    var occlusion = 0.0;
    let radius = 0.5; // Tweak this value depending on your scene scale
    let bias = 0.025; // Tweak to prevent self-shadowing acne on flat surfaces

    for (var i = 0u; i < 64u; i = i + 1u) {
        // Get the precalculated tangent/view-space sample offset vector
        var sample_offset = ssao.samples[i].xyz;
        
        // Flip the sample vector if it points against our estimated normal orientation
        if (dot(sample_offset, normal) < 0.0) {
            sample_offset = -sample_offset;
        }
        
        // Calculate the sample point position in view space
        let sample_pos = view_pos + sample_offset * radius;
        
        // Project the sample point back to screen space UV coords
        let offset_clip = uniforms.camera_proj * vec4f(sample_pos, 1.0);
        var offset_ndc = offset_clip.xy / offset_clip.w;
        // Transform NDC range [-1, 1] to UV range [0, 1]
        let sample_uv = vec2f(offset_ndc.x * 0.5 + 0.5, 1.0 - (offset_ndc.y * 0.5 + 0.5));
        
        // Sample the real geometry depth at this sample point's UV location
        let sample_raw_depth = textureSample(depth, depth_sampler, sample_uv);
        let sample_linear_depth = linearize_depth(sample_raw_depth);
        
        // Apply range check to prevent far away background elements from causing occlusion artefacts
        let range_check = smoothstep(0.0, 1.0, radius / abs(view_pos.z - sample_linear_depth));
        
        // Accumulate occlusion if the sample point is behind the geometry surface depth
        if (sample_linear_depth >= -sample_pos.z + bias) {
            occlusion += 1.0 * range_check;
        }
    }
    
    // Normalize occlusion factor and invert it to get an intensity multiplier
    let ao_factor = 1.0 - (occlusion / 64.0);

    // 3. APPLY LIGHT MODULATION & EXTRA EFFECTS
    // Modulate ambient color properties with the calculated ao factor
    let lit_color = vec4f(color.rgb * ao_factor, color.a);
    
    let linear_depth = linearize_depth(raw_depth);
    var fog_factor = simpleFog(linear_depth);
    
    var final_color = mix(lit_color, uniforms.fog_color, fog_factor);
    
    if (uniforms.underwater == 1) {
        final_color = mix(final_color, vec4f(0.0, 0.0, 1.0, 1.0), 0.5);
    }
    
    return final_color;
}