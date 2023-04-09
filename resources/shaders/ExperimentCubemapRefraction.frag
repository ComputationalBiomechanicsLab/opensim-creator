#version 330 core

uniform vec3 uCameraPos;
uniform samplerCube uSkybox;
uniform float uIOR = 1.52;

in vec3 Normal;
in vec3 Position;

out vec4 FragColor;

void main()
{
    float ratio = 1.00/uIOR;
    vec3 I = normalize(Position - uCameraPos);
    vec3 R = refract(I, normalize(Normal), ratio);
    FragColor = vec4(texture(uSkybox, R).rgb, 1.0);
}
