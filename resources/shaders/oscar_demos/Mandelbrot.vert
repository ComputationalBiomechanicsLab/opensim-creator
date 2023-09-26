#version 330 core

uniform mat4 uModelMat;
uniform mat4 uViewProjMat;

layout (location = 0) in vec3 aPos;

out vec2 aFragPos;

void main()
{
    gl_Position = uViewProjMat * uModelMat * vec4(aPos, 1.0);
    aFragPos = gl_Position.xy;
}
