#version 330 core

uniform mat4 uModelMat;
uniform mat4 uViewMat;
uniform mat4 uProjMat;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 FragTexCoord;

void main()
{
    gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
    FragTexCoord = aTexCoord;
}
