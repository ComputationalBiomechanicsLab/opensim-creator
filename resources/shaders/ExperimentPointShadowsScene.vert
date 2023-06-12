#version 330 core

uniform mat4 uViewProjMat;
uniform mat3 uNormalMat;
uniform mat4 uModelMat;
uniform bool uReverseNormals;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

void main()
{
    vec4 fragAffineWorldPos = uModelMat * vec4(aPos, 1.0);
    vs_out.FragPos = vec3(fragAffineWorldPos);
    vs_out.Normal = uNormalMat * (uReverseNormals ? -aNormal : aNormal);
    vs_out.TexCoords = aTexCoord;
    gl_Position = uViewProjMat * fragAffineWorldPos;
}