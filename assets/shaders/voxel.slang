import terrain.defs;

struct PushConstants
{
    float4x4 view_matrix;
    float4x4 model_matrix;
    float time;
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

Texture2DArray images;
SamplerState images_sampler;

StructuredBuffer<float4> positions;
StructuredBuffer<BlockState> blocks;
StructuredBuffer<uint> visibilityBuffer;

StructuredBuffer<uint4> textureRegistry;

[push_constant]
PushConstants constants;

// uint getVisibility(uint idx)
// {
//     uint e = idx >> 4;
//     uint w = (idx >> 2) & 3;
//     uint b = idx & 3;
//     return (visibilityBuffer[e][w] >> (b * 8)) & 0xFFu;
// }

[shader("vertex")]
VertexOutput vertexMain(VertexInput vertex)
{
    VertexOutput out;
    out.gradient_color = float3(0.0, 0.0, 0.0);

    BlockState block = blocks[vertex.instance_id];
    float3 position = positions[vertex.instance_id].xyz;
    // uint visibility = getVisibility(vertex.instance_id);
    uint visibility = visibilityBuffer[vertex.instance_id];

    // if (block.id_raw == 0
    //     || (((visibility & (1 << 1)) == 0 && vertex.index >= 0 && vertex.index < 4) ||
    //         ((visibility & (1 << 0)) == 0 && vertex.index >= 4 && vertex.index < 8) ||
    //         ((visibility & (1 << 2)) == 0 && vertex.index >= 8 && vertex.index < 12) ||
    //         ((visibility & (1 << 3)) == 0 && vertex.index >= 12 && vertex.index < 16) ||
    //         ((visibility & (1 << 4)) == 0 && vertex.index >= 16 && vertex.index < 20) ||
    //         ((visibility & (1 << 5)) == 0 && vertex.index >= 20 && vertex.index < 24)))
    //     return out;

    if (block.isAir())
        return out;

    // NOTE: Display Y by Y
    if (constants.time < position.y * 0.05)
        return out;

    // NOTE: Make the chunk rise from bellow the ground
    // position.y -= 128.0 - (min(constants.time, 1.0) / 1.0) * 128.0;

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

    uint4 encodedTextureIndices = textureRegistry[(block.id_raw & 0xFFFF) - 1];
    uint textureIndices[6] = { encodedTextureIndices.x & 0xFFFF, encodedTextureIndices.x >> 16, encodedTextureIndices.y & 0xFFFF, encodedTextureIndices.y >> 16, encodedTextureIndices.z & 0xFFFF, encodedTextureIndices.z >> 16 };
    out.texture_index = textureIndices[vertex.index / 4];

    return out;
}

inline bool isGrayscale(float4 color)
{
    return color.r == color.g && color.g == color.b;
}

inline bool isBlack(float3 color)
{
    return color.r == 0.0 && color.g == 0.0 && color.b == 0.0;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput in)
{
    float2 uv2 = in.uv;
    uv2.y = 1.0 - uv2.y;

    float4 color = images.Sample(images_sampler, float3(uv2, float(in.texture_index)));

    if (isGrayscale(color) && !isBlack(in.gradient_color))
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
