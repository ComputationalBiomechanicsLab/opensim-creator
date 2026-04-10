#version 330 core

uniform mat4 uModelMat;
uniform mat3 uNormalMat;
uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(uModelMat * vec4(aPos, 1.0));
    Normal = uNormalMat * aNormal;

    gl_Position = uViewProjMat * vec4(FragPos, 1.0);
}
