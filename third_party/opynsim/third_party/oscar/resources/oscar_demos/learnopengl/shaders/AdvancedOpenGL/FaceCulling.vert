#version 330 core

uniform mat4 uViewProjMat;
uniform mat4 uModelMat;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 FragTexCoord;

void main()
{
    gl_Position = uViewProjMat * uModelMat * vec4(aPos, 1.0);
    FragTexCoord = aTexCoord;
}
