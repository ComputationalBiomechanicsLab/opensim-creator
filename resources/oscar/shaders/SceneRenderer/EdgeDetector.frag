#version 330 core

uniform sampler2D uScreenTexture;
uniform vec4 uRim0Color;
uniform vec4 uRim1Color;
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
    vec2 rim0XY = vec2(0.0, 0.0);
    vec2 rim1XY = vec2(0.0, 0.0);
    for (int i = 0; i < g_KernelCoefficients.length(); ++i) {
        vec2 offset = uRimThickness * g_TextureOffsets[i];
        vec2 coord = TexCoords + offset;
        vec2 v = texture(uScreenTexture, coord).rg;
        rim0XY += v.r * g_KernelCoefficients[i];
        rim1XY += v.g * g_KernelCoefficients[i];
    }

    // the maximum value from the Sobel Kernel is sqrt(3^2 + 3^2) == sqrt(18)
    //
    // but lowering the scaling factor a bit is handy for making the rims more solid
    float rim0Strength = length(rim0XY) / 4.242640;
    float rim1Strength = length(rim1XY) / 4.242640;

    vec4 rim0Color = rim0Strength * uRim0Color;
    vec4 rim1Color = rim1Strength * uRim1Color;

    FragColor = rim0Color + rim1Color;
}
