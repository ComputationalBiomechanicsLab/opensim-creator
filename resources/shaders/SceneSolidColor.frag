#version 330 core

uniform vec4 uDiffuseColor;
out vec4 FragColor;

void main()
{
    FragColor = uDiffuseColor;
}