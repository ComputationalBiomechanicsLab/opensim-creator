#version 330 core

uniform mat4 uModelMat;
uniform mat4 uViewProjMat;
uniform mat3 uNormalMat;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

void main()
{
    vec4 worldPos = uModelMat * vec4(aPos, 1.0);

    FragPos = worldPos.xyz;
    TexCoords = aTexCoord;
    Normal = uNormalMat * aNormal;
    gl_Position = uViewProjMat * worldPos;
}
