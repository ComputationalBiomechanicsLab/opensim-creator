#version 330 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D uSampler0;

void main(void) {
    fragColor = texture(uSampler0, texCoord);
}
