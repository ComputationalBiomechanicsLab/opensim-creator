#version 330 core

uniform mat4 uMVP;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord;

void main(void) {
    gl_Position = uMVP * vec4(aPos, 1.0f);
    texCoord = aTexCoord;
}
