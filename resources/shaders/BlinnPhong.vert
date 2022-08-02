#version 330 core

uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;
layout (location = 6) in mat4 aModelMat;
layout (location = 10) in mat3 aNormalMat;

out vec3 FragPosWorld;
out vec3 NormalWorld;

void main()
{
    FragPosWorld = vec3(aModelMat * vec4(aPos, 1.0));
    NormalWorld = aNormalMat * aNormal;
    gl_Position = uViewProjMat * aModelMat * vec4(aPos, 1.0);
}