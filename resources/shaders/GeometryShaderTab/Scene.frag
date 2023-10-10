#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform vec4 uDiffuseColor;

void main()
{
    FragColor = uDiffuseColor;
}
