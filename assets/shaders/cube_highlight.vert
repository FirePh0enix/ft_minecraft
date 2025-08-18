#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform uniforms
{
    mat4 modelMatrix;
    vec3 color;
};

layout(push_constant) uniform constants
{
    mat4 viewMatrix;
};

void main()
{
    gl_Position = viewMatrix * modelMatrix * vec4(position, 1.0);
    fragColor = color;
}
