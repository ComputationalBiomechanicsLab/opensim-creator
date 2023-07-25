#version 330 core

uniform vec3 uLightPos;
uniform float uFarPlane;

in vec4 WorldFragPos;
out vec4 FragColor;

void main()
{
    float lightDistance = length(WorldFragPos.xyz - uLightPos);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / uFarPlane;
    
    // write this as modified depth
    FragColor.r = lightDistance;
}