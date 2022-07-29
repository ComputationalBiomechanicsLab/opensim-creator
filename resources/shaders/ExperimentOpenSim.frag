#version 330 core

uniform vec4 uDiffuseColor = vec4(1.0, 1.0, 1.0, 1.0);

in vec4 GouraudBrightness;

out vec4 Color0Out;

void main()
{
    Color0Out = GouraudBrightness * uDiffuseColor;
}