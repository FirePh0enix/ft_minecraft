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


struct SSAO {
    // Match SSAOUniforms in Renderer.hpp; vec4 keeps the uniform array stride
    // at 16 bytes even though only XYZ participates in hemisphere sampling.
    samples: array<vec4f, 64>,
}
@group(0) @binding(0) var<uniform> uniforms: PostProcess;
@group(0) @binding(1) var<uniform> ssao: SSAO;
@group(0) @binding(2) var depth: texture_depth_2d;
@group(0) @binding(3) var depth_sampler: sampler;

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) clip_position: vec2f,
}

@vertex
fn vertex_main(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    // Generate one oversized triangle covering the target without a vertex
    // buffer. UV Y is flipped because texture coordinates increase downward.
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

// Helper function to get view-space position from UV and depth
fn get_view_pos(uv: vec2f, raw_depth: f32) -> vec3f {
    // Convert UV and depth to Normalized Device Coordinates (NDC)
    // WebGPU depth is [0.0, 1.0]
    let ndc = vec4f(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0, raw_depth, 1.0);
    let view_pos_clip = uniforms.inverse_camera_proj * ndc;
    // Undo the perspective divide to recover distances in world-unit scale.
    return view_pos_clip.xyz / view_pos_clip.w;
}

// Reconstruct at texel centers; filtering depth invents surfaces at silhouettes.
fn view_position_at(pixel: vec2i) -> vec3f {
    let size = vec2i(textureDimensions(depth));
    let p = clamp(pixel, vec2i(0), size - vec2i(1));
    return get_view_pos((vec2f(p) + 0.5) / vec2f(size), textureLoad(depth, p, 0));
}

fn ambient_visibility(pixel: vec2i, position: vec3f) -> f32 {
    // Choose the neighbour on the same surface instead of differentiating
    // across a foreground/background edge (or across a fragment quad).
    let left = position - view_position_at(pixel - vec2i(1, 0));
    let right = view_position_at(pixel + vec2i(1, 0)) - position;
    let up = position - view_position_at(pixel - vec2i(0, 1));
    let down = view_position_at(pixel + vec2i(0, 1)) - position;
    let dx = select(right, left, abs(left.z) < abs(right.z));
    let dy = select(down, up, abs(up.z) < abs(down.z));
    let cross_normal = cross(dx, dy);
    // Clamped neighbours can coincide at the viewport boundary. Avoid
    // normalizing a zero vector and propagating NaNs through the AO buffer.
    if (dot(cross_normal, cross_normal) < 1e-16) {
        return 1.0;
    }
    var normal = normalize(cross_normal);
    // Screen-space Y affects the cross-product orientation. The visible
    // surface normal must point toward the camera (the view-space origin).
    normal = select(normal, -normal, dot(normal, -position) < 0.0);

    // Rotate the hemisphere into the surface's tangent frame. A coherent,
    // stratified kernel avoids adding random per-pixel or per-frame noise.
    // Choose an axis away from parallel to the normal so the tangent is valid.
    let axis = select(vec3f(0.0, 0.0, 1.0), vec3f(0.0, 1.0, 0.0), abs(normal.z) > 0.9);
    let tangent = normalize(cross(axis, normal));
    let basis = mat3x3f(tangent, cross(normal, tangent), normal);
    let radius = 1.0; // World units: one block, for readable contact shadows.
    // Require a small separation before counting an occluder to suppress
    // self-occlusion from depth precision and texel-center reconstruction.
    let bias = 0.025;
    let size = vec2i(textureDimensions(depth));
    var occlusion = 0.0;
    for (var i = 0u; i < 64u; i += 1u) {
        let sample_pos = position + basis * ssao.samples[i].xyz * radius;
        let clip = uniforms.camera_proj * vec4f(sample_pos, 1.0);
        // Reject samples behind the eye or outside WebGPU's [0, W] clip Z.
        if (clip.w <= 0.0 || clip.z <= 0.0 || clip.z >= clip.w) {
            continue;
        }
        let ndc = clip.xy / clip.w;
        let uv = vec2f(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
        if (any(uv < vec2f(0.0)) || any(uv >= vec2f(1.0))) {
            continue;
        }
        let p = vec2i(uv * vec2f(size));
        let raw = textureLoad(depth, p, 0);
        if (raw >= 1.0) {
            continue;
        }
        let surface_pos = get_view_pos((vec2f(p) + 0.5) / vec2f(size), raw);
        // Nearby in screen space does not imply nearby in the world. Fade
        // distant geometry out so background surfaces cannot cast AO halos.
        // Both values are signed view-space Z. A larger Z is closer to
        // the camera, so it occludes a sample behind it.
        let range_weight = 1.0 - smoothstep(radius * 0.5, radius * 2.0, length(surface_pos - position));
        if (surface_pos.z >= sample_pos.z + bias) {
            occlusion += range_weight;
        }
    }
    // Rejected samples remain unoccluded in the fixed denominator, preventing
    // stronger shadows near screen edges. The exponent controls AO contrast.
    return pow(clamp(1.0 - occlusion / 64.0, 0.0, 1.0), 1.8);
}


@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec2f {
    // One full-resolution texel per 2x2 block. Use the same mapping during
    // upsampling, including the partial block at odd viewport dimensions.
    let size = vec2i(textureDimensions(depth));
    let pixel = min(vec2i(in.position.xy) * 2 + vec2i(1), size - vec2i(1));
    let raw = textureLoad(depth, pixel, 0);
    if (raw >= 1.0) {
        // Neutral visibility plus a depth sentinel rejected by upsampling.
        return vec2f(1.0, 0.0);
    }
    let position = view_position_at(pixel);
    return vec2f(ambient_visibility(pixel, position), -position.z);
}
