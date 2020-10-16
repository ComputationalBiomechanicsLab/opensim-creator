#version 330 core

uniform mat4 projMat;
uniform mat4 viewMat;
uniform mat4 modelMat;
uniform mat4 normalMat;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out vec2 texCoords;

void main(void) {
    gl_Position = vec4(aPos, 1.0f);
    texCoords = aTexCoords;
}
