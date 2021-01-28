#version 330 core

in vec2 texCoord;

layout (location = 0) out vec4 fragColor;

uniform sampler2D uSampler0;
uniform mat4 uSamplerMultiplier = mat4(1.0);

void main(void) {
    fragColor = uSamplerMultiplier * texture(uSampler0, texCoord);
}
