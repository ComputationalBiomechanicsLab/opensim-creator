#version 330 core

uniform sampler2D uScreenTexture;
uniform vec4 uRimRgba;
uniform vec2 uRimThickness;

in vec2 TexCoords;
out vec4 FragColor;

// sampling offsets to use when retrieving samples to feed
// into the kernel
const vec2 g_TextureOffsets[9] = vec2[](
    vec2(-1.0,  1.0), // top-left
    vec2( 0.0,  1.0), // top-center
    vec2( 1.0,  1.0), // top-right
    vec2(-1.0,  0.0), // center-left
    vec2( 0.0,  0.0), // center-center
    vec2( 1.0,  0.0), // center-right
    vec2(-1.0, -1.0), // bottom-left
    vec2( 0.0, -1.0), // bottom-center
    vec2( 1.0, -1.0)  // bottom-right
);

// https://computergraphics.stackexchange.com/questions/2450/opengl-detection-of-edges
//
// this is known as a "Sobel Kernel"
const vec2 g_KernelCoefficients[9] = vec2[](
    vec2( 1.0,  1.0),  // top-left
    vec2( 0.0,  2.0),  // top-center
    vec2(-1.0,  1.0),  // top-right

    vec2( 2.0,  0.0),  // center-left
    vec2( 0.0,  0.0),  // center
    vec2(-2.0,  0.0),  // center-right

    vec2( 1.0, -1.0),  // bottom-left
    vec2( 0.0, -2.0),  // bottom-center
    vec2(-1.0, -1.0)   // bottom-right
);

void main(void)
{
    vec2 rimXY = vec2(0.0, 0.0);
    for (int i = 0; i < g_KernelCoefficients.length(); ++i)
    {
        vec2 offset = uRimThickness * g_TextureOffsets[i];
        vec2 coord = TexCoords + offset;

        float v = texture(uScreenTexture, coord).r;
        rimXY += v * g_KernelCoefficients[i];
    }

    // the maximum value from the Sobel Kernel is sqrt(3^2 + 3^2) == sqrt(18)
    //
    // but lowering the scaling factor a bit is handy for making the rims more solid
    float rimStrength = length(rimXY) / 4.242640;

    FragColor = vec4(uRimRgba.rgb, clamp(rimStrength * uRimRgba.a, 0.0, 1.0));
}
