#version 330 core

uniform mat4 uModelMat;
uniform mat4 uViewProjMat;
uniform vec2 uTextureOffset = vec2(0.0, 0.0);
uniform vec2 uTextureScale = vec2(1.0, 1.0);

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoords;

void main()
{
    TexCoords = uTextureOffset + (uTextureScale * aTexCoord);
    gl_Position = uViewProjMat * uModelMat * vec4(aPos, 1.0);
}
