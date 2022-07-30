#version 330 core

uniform sampler2D uScreenTexture;
uniform vec4 uRimRgba;
uniform float uRimThickness;

in vec2 TexCoords;
out vec4 FragColor;

// sampling offsets to use when retrieving samples to feed
// into the kernel
const vec2 offsets[9] = vec2[](
    vec2(-1.0f,  1.0f), // top-left
    vec2( 0.0f,  1.0f), // top-center
    vec2( 1.0f,  1.0f), // top-right
    vec2(-1.0f,  0.0f), // center-left
    vec2( 0.0f,  0.0f), // center-center
    vec2( 1.0f,  0.0f), // center-right
    vec2(-1.0f, -1.0f), // bottom-left
    vec2( 0.0f, -1.0f), // bottom-center
    vec2( 1.0f, -1.0f)  // bottom-right
);

// https://computergraphics.stackexchange.com/questions/2450/opengl-detection-of-edges
const float xkern[9] = float[](
    +1.0, 0.0, -1.0,
    +2.0, 0.0, -2.0,
    +1.0, 0.0, -1.0
);

const float ykern[9] = float[](
    +1.0, +2.0, +1.0,
        0.0,  0.0,  0.0,
    -1.0, -2.0, -1.0
);

void main(void)
{
    float rimX = 0.0;
    float rimY = 0.0;
    for (int i = 0; i < xkern.length(); ++i) {
        vec2 offset = uRimThickness * offsets[i];
        vec2 coord = TexCoords + offset;

        float v = texture(uScreenTexture, coord).r;
        float x = xkern[i] * v;
        float y = ykern[i] * v;

        rimX += x;
        rimY += y;
    }

    float rimStrength = sqrt(rimX*rimX + rimY*rimY) / 3.0f;

    // rimStrength = abs(rimStrength);  // for inner edges

    FragColor = vec4(uRimRgba.rgb, rimStrength * uRimRgba.a);
}