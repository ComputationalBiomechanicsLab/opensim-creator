#version 330 core

uniform mat4 uModelMat;
uniform mat4 uViewProjMat;
uniform mat3 uNormalMat;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec3 FragWorldPos;
out vec3 NormalWorldDir;
out vec2 TexCoord;

void main()
{
    FragWorldPos = vec3(uModelMat * vec4(aPos, 1.0));
    NormalWorldDir = normalize(uNormalMat * aNormal);
    TexCoord = aTexCoord;
    gl_Position = uViewProjMat * vec4(FragWorldPos, 1.0);
}