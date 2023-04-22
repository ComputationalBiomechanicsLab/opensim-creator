#version 330 core

uniform vec4 uLightColor;

out vec4 FragColor;

void main()
{
    FragColor = uLightColor;
}