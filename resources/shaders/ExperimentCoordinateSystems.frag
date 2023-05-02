#version 330 core

uniform sampler2D uTexture1;
uniform sampler2D uTexture2;

in vec2 FragTexCoord;
out vec4 FragColor;

void main()
{
    FragColor = mix(texture(uTexture1, FragTexCoord), texture(uTexture2, FragTexCoord), 0.1);
}