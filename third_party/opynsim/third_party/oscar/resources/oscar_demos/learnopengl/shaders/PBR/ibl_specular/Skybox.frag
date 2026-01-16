#version 330 core

uniform samplerCube uEnvironmentMap;

in vec3 WorldPos;

out vec4 FragColor;

void main()
{
    vec3 envColor = texture(uEnvironmentMap, WorldPos).rgb;

    // HDR tonemap
    envColor = envColor / (envColor + vec3(1.0));

    FragColor = vec4(envColor, 1.0);
}
