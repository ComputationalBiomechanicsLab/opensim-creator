#version 330 core

uniform vec3 uLightPos;
uniform float uFarPlane;

in vec4 FragPos;

void main()
{
    float lightDistance = length(FragPos.xyz - uLightPos);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / uFarPlane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}