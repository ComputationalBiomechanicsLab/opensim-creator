#version 330 core

uniform sampler2D uDiffuseTexture;

in vec2 TexCoord;
in vec4 GouraudBrightness;
out vec4 Color0Out;

void main()
{
    Color0Out = GouraudBrightness * texture(uDiffuseTexture, TexCoord);
}