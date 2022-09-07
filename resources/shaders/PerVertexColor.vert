#version 330 core

uniform mat4 uProjMat;
uniform mat4 uViewMat;
uniform mat4 uModelMat;

layout (location = 0) in vec3 aPos;
layout (location = 3) in vec4 aColor;

out vec4 aVertColor;

void main()
{
    gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
    aVertColor = aColor;
}