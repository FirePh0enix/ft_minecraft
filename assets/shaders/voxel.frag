#version 450 core

layout(location = 0) in vec4 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 lightVec;
layout(location = 4) in flat uint textureIndex;
layout(location = 5) in vec3 gradientColor;

layout(location = 0) out vec4 pixelColor;

layout(set = 0, binding = 0) uniform sampler2DArray images;

bool is_grayscale(vec4 color)
{
    return color.r == color.g && color.g == color.b;
}

bool is_black(vec3 color)
{
    return color.r == 0.0 && color.g == 0.0 && color.b == 0.0;
}

void main()
{
    vec2 uv2 = uv;
    uv2.y = 1.0 - uv2.y;

    vec4 color = texture(images, vec3(uv2, float(textureIndex)));

    if (is_grayscale(color) && !is_black(gradientColor))
    {
        color *= vec4(gradientColor, 1.0);
    }

    vec3 N = normalize(normal);
    vec3 L = normalize(lightVec);
    vec3 V = normalize(pos.xyz);
    vec3 R = normalize(-reflect(L, N));
    vec3 diffuse = max(dot(N, -L), 0.1) * color.rgb;

    pixelColor = vec4(diffuse, 1.0) * color; // FIXME: ???
}
