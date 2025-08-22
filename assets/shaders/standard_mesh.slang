struct PushConstants
{
    float4x4 view_matrix;
    float4x4 model_matrix;
};

struct Vertex
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct VsOutput
{
    float4 position : SV_Position;
};

struct MeshData
{
    float4x4 model_matrix;
};

struct MaterialData
{
    float3 base_color;
};

// ConstantBuffer<MeshData> mesh;
ConstantBuffer<MaterialData> material;

// TODO: No all API support push constants. On most API constants should be split between world data (view_matrix) and
//       model data (model_matrix) since one buffer is the same for all models and one will change per model.
//       Then on vulkan world data could still be passed to push_constant or uniform on other platforms.
[push_constant]
PushConstants constants;

[shader("vertex")]
VsOutput vertexMain(Vertex vertex)
{
    VsOutput out;
    out.position = mul(constants.view_matrix, mul(constants.model_matrix, float4(vertex.position, 1.0)));
    return out;
}

[shader("fragment")]
float4 fragmentMain()
{
    return float4(material.base_color, 1.0);
}
