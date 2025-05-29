#version 450

layout(location = 0) in vec2 inTextCoords;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform sampler2D bitmap;

void main(){
    outFragColor = inColor * vec4(1.0, 1.0, 1.0, texture(bitmap, inTextCoords).r);
}
