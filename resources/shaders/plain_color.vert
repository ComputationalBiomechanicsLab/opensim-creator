#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 uModelMat;
uniform mat4 uViewMat;
uniform mat4 uProjMat;

void main(void) {
    gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
}
