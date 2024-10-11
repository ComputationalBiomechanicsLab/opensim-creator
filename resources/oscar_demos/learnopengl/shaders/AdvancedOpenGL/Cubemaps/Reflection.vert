#version 330 core

uniform mat4 uModelMat;
uniform mat3 uNormalMat;
uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;

out vec3 Normal;
out vec3 Position;

void main()
{
    vec4 pos = uModelMat * vec4(aPos, 1.0);

    Normal = uNormalMat * aNormal;
    Position = vec3(pos);
    gl_Position = uViewProjMat * pos;
}
