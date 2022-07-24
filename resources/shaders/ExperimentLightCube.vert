#version 330 core

uniform mat4 uModelMat;
uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = uViewProjMat * uModelMat * vec4(aPos, 1.0);
}