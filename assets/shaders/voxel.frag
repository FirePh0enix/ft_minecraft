#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 lightVec;
layout(location = 4) flat in uint textureIndex;
layout(location = 5) flat in uint gradient;
layout(location = 6) flat in uint gradientType;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2DArray images;

#define ambient 0.1

bool isGrayscale(vec4 color) {
    return color.r == color.g && color.g == color.b;
}

void main() {
    vec2 uv2 = uv;
    uv2.y = 1.0 - uv2.y;
    
    vec4 color = texture(images, vec3(uv2, float(textureIndex)));

    if (isGrayscale(color) && gradientType > 0) {
        color *= vec4(13.0 / 255.0, 94.0 / 255.0, 21.0 / 255.0, 1.0);
    }

    vec3 N = normalize(normal);
    vec3 L = normalize(lightVec);
    vec3 V = normalize(position.xyz);
    vec3 R = normalize(-reflect(L, N));
    vec3 diffuse = max(dot(N, -L), ambient) * color.rgb;

    // outColor = vec4(diffuse, color.a);
    outColor = color;
}
