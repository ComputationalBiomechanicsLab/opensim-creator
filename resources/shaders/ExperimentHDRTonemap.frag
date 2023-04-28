#version 330 core

uniform sampler2D uTexture;
uniform bool uUseTonemap;
uniform float uExposure;

in vec2 TexCoords;

out vec4 FragColor;

void main()
{
    vec3 hdrColor = texture(uTexture, TexCoords).rgb;

    if(uUseTonemap)
    {
        // reinhard
        // vec3 result = hdrColor / (hdrColor + vec3(1.0));
        // exposure
        vec3 result = vec3(1.0) - exp(-hdrColor * uExposure);
        FragColor = vec4(result, 1.0);
    }
    else
    {
        FragColor = vec4(hdrColor, 1.0);
    }
}
