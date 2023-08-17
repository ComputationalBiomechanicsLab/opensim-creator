#version 330 core

uniform mat4 uViewProjMat;
uniform mat4 uModelMat;
uniform mat3 uNormalMat;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 WorldPos;
out vec3 Normal;

void main()
{
    vec4 affinePos = uModelMat * vec4(aPos, 1.0);

    TexCoord = aTexCoord;
    WorldPos = vec3(affinePos);
    Normal = uNormalMat * aNormal;
    gl_Position = uViewProjMat * vec4(WorldPos, 1.0);
}