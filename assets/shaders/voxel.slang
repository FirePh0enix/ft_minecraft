static const float3 plains_grass_color = float3(63.0 / 255.0, 155.0 / 255.0, 11.0 / 255.0);

static const float3 grass_colors[14] = {
    plains_grass_color, // FrozenRiver
    plains_grass_color, // River
    plains_grass_color, // StonyShore
    plains_grass_color, // StonyPeaks
    plains_grass_color, // JaggedPeaks
    plains_grass_color, // FrozenPeaks
    plains_grass_color, // Beach
    plains_grass_color, // Desert
    plains_grass_color, // SnowyPlains
    plains_grass_color, // Taiga
    plains_grass_color, // Plains
    plains_grass_color, // Savanna
    plains_grass_color, // FrozenOcean
    plains_grass_color, // Ocean
};

enum GradientType
{
    None = 0,
    Grass = 1,
    Water = 2
};

struct PushConstants
{
    float4x4 view_matrix;
};

struct Vertex
{
    uint index : SV_VulkanVertexID;

    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;

    float3 instance_position;
    uint3 textures;
    uint visibility_biome_gradient;
};

struct VsOutput
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

[push_constant]
PushConstants constants;

[shader("vertex")]
VsOutput vertexMain(Vertex vertex)
{
    uint visibility = vertex.visibility_biome_gradient & 0xFF;
    uint biome = (vertex.visibility_biome_gradient >> 8) & 0xFF;
    GradientType gradient_type = GradientType((vertex.visibility_biome_gradient >> 16) & 0XFF);

    VsOutput out;
    out.gradient_color = float3(0.0, 0.0, 0.0);

    // if (((visibility & (1 << 1)) == 0 && vertex.index >= 0 && vertex.index < 4) ||
    //     ((visibility & (1 << 0)) == 0 && vertex.index >= 4 && vertex.index < 8) ||
    //     ((visibility & (1 << 2)) == 0 && vertex.index >= 8 && vertex.index < 12) ||
    //     ((visibility & (1 << 3)) == 0 && vertex.index >= 12 && vertex.index < 16) ||
    //     ((visibility & (1 << 4)) == 0 && vertex.index >= 16 && vertex.index < 20) ||
    //     ((visibility & (1 << 5)) == 0 && vertex.index >= 20 && vertex.index < 24))
    // {
    //     // float nan = 0.0 / 0.0;
    //     // out.position = float4(nan, nan, nan, nan);
    //     return out;
    // }

    // float4x4 model_matrix = float4x4(
    //     1.0, 0.0, 0.0, 0.0,
    //     0.0, 1.0, 0.0, 0.0,
    //     0.0, 0.0, 1.0, 0.0,
    //     vertex.instance_position.x, vertex.instance_position.y, vertex.instance_position.z, 1.0
    // );

    float4x4 model_matrix = float4x4(
        1.0, 0.0, 0.0, vertex.instance_position.x,
        0.0, 1.0, 0.0, vertex.instance_position.y,
        0.0, 0.0, 1.0, vertex.instance_position.z,
        0.0, 0.0, 0.0, 1.0
    );

    out.world_position = mul(model_matrix, float4(vertex.position, 1.0));
    out.position = mul(constants.view_matrix, out.world_position);

    switch (gradient_type)
    {
    case GradientType.Grass:
        out.gradient_color = grass_colors[biome];
        break;
    case GradientType.Water:
        break;
    }

    out.uv = vertex.uv;
    out.normal = vertex.normal;
    out.light_vec = float3(-1.0, -1.0, 0.0);

    uint texture_indices[6] = { vertex.textures.x & 0xFFFF, vertex.textures.x >> 16, vertex.textures.y & 0xFFFF, vertex.textures.y >> 16, vertex.textures.z & 0xFFFF, vertex.textures.z >> 16 };
    out.texture_index = texture_indices[vertex.index / 4];

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
float4 fragmentMain(VsOutput in)
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
