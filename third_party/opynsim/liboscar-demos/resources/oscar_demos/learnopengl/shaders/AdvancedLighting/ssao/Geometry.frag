#version 330 core

in vec3 FragPos;
in vec2 TexCoords;
in vec3 Normal;

layout (location = 0) out vec4 gAlbedo;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gPosition;

void main()
{
    gAlbedo.rgb = vec3(0.95);
    gNormal = normalize(Normal);
    gPosition = FragPos;
}
