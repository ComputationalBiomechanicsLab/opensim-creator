#version 330 core

uniform mat4 uModelMat;
uniform mat4 uViewMat;
uniform mat4 uProjMat;

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

void main()
{
    vec4 pos = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);

    TexCoords = aPos;
    gl_Position = pos.xyww;
}
