#version 330 core

uniform sampler2D uHDRSceneRender;
uniform sampler2D uBloomBlur;
uniform bool uBloom;
uniform float uExposure;

in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    vec3 hdrSceneColor = texture(uHDRSceneRender, TexCoord).rgb;
    vec3 bloomColor = texture(uBloomBlur, TexCoord).rgb;

    if (uBloom)
    {
        hdrSceneColor += bloomColor;  // additive blending
    }

    // HDR tone mapping
    vec3 tonemappedColor = vec3(1.0) - exp(-hdrSceneColor * uExposure);

    FragColor = vec4(tonemappedColor, 1.0);
}
