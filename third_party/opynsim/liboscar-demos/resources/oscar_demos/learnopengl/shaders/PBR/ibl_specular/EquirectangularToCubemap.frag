#version 330 core

const vec2 c_InverseTan = vec2(0.1591, 0.3183);

uniform sampler2D uEquirectangularMap;

in vec3 WorldPos;

out vec4 FragColor;

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= c_InverseTan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(uEquirectangularMap, uv).rgb;

    FragColor = vec4(color, 1.0);
}
