#version 330 core

uniform vec3 uCameraPos;
uniform samplerCube uSkybox;

in vec3 Normal;
in vec3 Position;

out vec4 FragColor;

void main()
{
    vec3 I = normalize(Position - uCameraPos);
    vec3 R = reflect(I, normalize(Normal));
    FragColor = vec4(texture(uSkybox, R).rgb, 1.0);
}
