#version 450 core

vec3 plains_grass_color = vec3(63.0 / 255.0, 155.0 / 255.0, 11.0 / 255.0);

vec3 grass_colors[14] = {
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

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
// Instance inputs
layout(location = 3) in vec3 instancePosition;
layout(location = 4) in uvec3 textures;
layout(location = 5) in uint visibilityBiomeGradient;

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragLightVec;
layout(location = 4) out flat uint fragTextureIndex;
layout(location = 5) out vec3 fragGradientColor;

layout(push_constant) uniform constants
{
    mat4 viewMatrix;
};

void main()
{
    uint visibility = visibilityBiomeGradient & 0xFF;
    uint biome = (visibilityBiomeGradient >> 8) & 0xFF;
    uint gradientType = (visibilityBiomeGradient >> 16) & 0XFF;

    fragGradientColor = vec3(0.0, 0.0, 0.0);

    if (((visibility & (1 << 1)) == 0 && gl_VertexIndex >= 0 && gl_VertexIndex < 4) ||
        ((visibility & (1 << 0)) == 0 && gl_VertexIndex >= 4 && gl_VertexIndex < 8) ||
        ((visibility & (1 << 2)) == 0 && gl_VertexIndex >= 8 && gl_VertexIndex < 12) ||
        ((visibility & (1 << 3)) == 0 && gl_VertexIndex >= 12 && gl_VertexIndex < 16) ||
        ((visibility & (1 << 4)) == 0 && gl_VertexIndex >= 16 && gl_VertexIndex < 20) ||
        ((visibility & (1 << 5)) == 0 && gl_VertexIndex >= 20 && gl_VertexIndex < 24))
    {
        float nan = 0.0 / 0.0;
        gl_Position = vec4(nan, nan, nan, nan);
        return;
    }

    mat4 modelMatrix = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        instancePosition.x, instancePosition.y, instancePosition.z, 1.0
    );

    gl_Position = viewMatrix * modelMatrix * vec4(position, 1.0);

    // #ifndef DEPTH_PASS
    if (gradientType == 1) // Grass
    {
        fragGradientColor = grass_colors[biome];
    }
    else if (gradientType == 2) // Water
    {
    }

    fragPos = modelMatrix * vec4(position, 1.0);
    fragUV = uv;
    fragNormal = normal;
    fragLightVec = vec3(-1.0, -1.0, 0.0);

    uint textureIndices[6] = {textures.x & 0xFFFF, textures.x >> 16, textures.y & 0xFFFF, textures.y >> 16, textures.z & 0xFFFF, textures.z >> 16};
    fragTextureIndex = textureIndices[gl_VertexIndex / 4];
    // #endif
}
