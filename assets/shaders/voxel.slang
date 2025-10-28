struct PushConstants
{
    float4x4 view_matrix;
    float4x4 model_matrix;
};

struct VertexInput
{
    uint index : SV_VulkanVertexID;
    uint instance_id : SV_VulkanInstanceID;

    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct VertexOutput
{
    float4 position : SV_Position;

    float4 world_position;
    float2 uv;
    float3 normal;
    float3 light_vec;
    uint texture_index;
    float3 gradient_color;
};

// This structure need to be kept in sync with CPU code.
struct BlockState
{
    uint id_raw;
};

Texture2DArray images;
SamplerState images_sampler;

StructuredBuffer<float4> positions;
StructuredBuffer<BlockState> blocks;

[push_constant]
PushConstants constants;

[shader("vertex")]
VertexOutput vertexMain(VertexInput vertex)
{
    VertexOutput out;
    out.gradient_color = float3(0.0, 0.0, 0.0);

    BlockState block = blocks[vertex.instance_id];
    float3 position = positions[vertex.instance_id].xyz;

    if (block.id_raw == 0)
        return out;

    float4x4 model_matrix = float4x4(
        1.0, 0.0, 0.0, position.x,
        0.0, 1.0, 0.0, position.y,
        0.0, 0.0, 1.0, position.z,
        0.0, 0.0, 0.0, 1.0
    );

    out.world_position = mul(mul(model_matrix, constants.model_matrix), float4(vertex.position, 1.0));
    out.position = mul(constants.view_matrix, out.world_position);

    out.uv = vertex.uv;
    out.normal = vertex.normal;
    out.light_vec = float3(-1.0, -1.0, 0.0);

    // uint texture_indices[6] = { vertex.textures.x & 0xFFFF, vertex.textures.x >> 16, vertex.textures.y & 0xFFFF, vertex.textures.y >> 16, vertex.textures.z & 0xFFFF, vertex.textures.z >> 16 };
    // out.texture_index = texture_indices[vertex.index / 4];
    // TODO: texturing
    out.texture_index = 1;

    return out;
}

inline bool is_grayscale(float4 color)
{
    return color.r == color.g && color.g == color.b;
}

inline bool is_black(float3 color)
{
    return color.r == 0.0 && color.g == 0.0 && color.b == 0.0;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput in)
{
    float2 uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;

    float4 color = images.Sample(images_sampler, float3(uv2, float(in.texture_index)));

    if (is_grayscale(color) && !is_black(in.gradient_color))
    {
        color *= float4(in.gradient_color, 1.0);
    }

    float3 N = normalize(in.normal);
    float3 L = normalize(in.light_vec);
    float3 V = normalize(in.world_position.xyz);
    float3 R = normalize(-reflect(L, N));
    float3 diffuse = max(dot(N, -L), 0.1) * color.rgb;

    return float4(diffuse, 1.0);
}
