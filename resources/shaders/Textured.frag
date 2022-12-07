#version 330 core

uniform sampler2D uTextureSampler;

in vec2 TexCoord;
out vec4 FragColor;

void main()
{
    FragColor = vec4(texture(uTextureSampler, TexCoord).r, 0.0, 0.0, 1.0);
}