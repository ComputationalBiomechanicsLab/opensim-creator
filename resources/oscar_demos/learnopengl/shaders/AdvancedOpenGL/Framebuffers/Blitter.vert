#version 330 core

uniform mat4 uModelMat;
uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoord;
    gl_Position = uViewProjMat * uModelMat * vec4(aPos, 1.0);
}
