#version 330 core

uniform mat4 projMat;
uniform mat4 viewMat;
uniform mat4 modelMat;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord;

void main(void) {
    gl_Position = projMat * viewMat * modelMat * vec4(aPos, 1.0f);
    texCoord = aTexCoord;
}
