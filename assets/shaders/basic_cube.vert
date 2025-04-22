#version 450

// Per vertex data
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

// Per instance data
layout(location = 3) in vec3 instancePosition;
layout(location = 4) in vec3 texture0;
layout(location = 5) in vec3 texture1;
layout(location = 6) in uint visibility;

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragLightVec;
layout(location = 4) out vec4 fragShadowCoords;
layout(location = 5) out uint textureIndex;

layout(push_constant) uniform PushConstants {
    mat4 viewMatrix;
};

layout(binding = 2) uniform LightData {
    mat4 matrix;
    vec3 position;
} light;

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
);

void main() {
    // Discard vertices by setting the position to nan, the GPU will ignore them.
    if (
        ((visibility & (1 << 1)) == 0 && gl_VertexIndex >= 0 && gl_VertexIndex < 4) ||
        ((visibility & (1 << 0)) == 0 && gl_VertexIndex >= 4 && gl_VertexIndex < 8) ||
        ((visibility & (1 << 2)) == 0 && gl_VertexIndex >= 8 && gl_VertexIndex < 12) ||
        ((visibility & (1 << 3)) == 0 && gl_VertexIndex >= 12 && gl_VertexIndex < 16) ||
        ((visibility & (1 << 4)) == 0 && gl_VertexIndex >= 16 && gl_VertexIndex < 20) ||
        ((visibility & (1 << 5)) == 0 && gl_VertexIndex >= 20 && gl_VertexIndex < 24)
    ) {
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

    fragPos = modelMatrix * vec4(position, 1.0);
    fragUV = uv;
    fragNormal = normal;
    fragLightVec = normalize(light.position - position);
    fragShadowCoords = biasMat * light.matrix * modelMatrix * vec4(position, 1.0);

    uint textureIndices[] = uint[](uint(texture0.x), uint(texture0.y), uint(texture0.z), uint(texture1.x), uint(texture1.y), uint(texture1.z));
    textureIndex = textureIndices[gl_VertexIndex / 4];
}
