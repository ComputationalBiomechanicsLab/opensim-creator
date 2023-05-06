#version 330 core

uniform sampler2D uDiffuseMap;
uniform sampler2D uSpecularMap;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

layout (location = 0) out vec4 gAlbedoSpecular;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gPosition;

void main()
{
    gAlbedoSpecular.rgb = texture(uDiffuseMap, TexCoords).rgb;
    gAlbedoSpecular.a = texture(uSpecularMap, TexCoords).r;
    gNormal = normalize(Normal);
    gPosition = FragPos;
}