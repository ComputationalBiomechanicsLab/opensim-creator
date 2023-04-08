#version 330 core

uniform samplerCube uSkybox;

in vec3 TexCoords;

out vec4 FragColor;

void main()
{
    FragColor = texture(uSkybox, TexCoords);
}
