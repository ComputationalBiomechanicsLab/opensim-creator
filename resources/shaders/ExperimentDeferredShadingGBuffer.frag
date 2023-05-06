#version 330 core

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

layout (location = 0) out vec4 gAlbedo;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gPosition;

void main()
{
    gAlbedo = vec4(1.0, 0.0, 0.0, 1.0);
    gNormal = normalize(Normal);
    gPosition = FragPos;
}