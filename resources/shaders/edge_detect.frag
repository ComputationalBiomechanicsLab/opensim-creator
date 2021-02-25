#version 330 core

// edge detect: sample a texture with edge detection

in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;

uniform sampler2D uSampler0;
uniform vec4 uRimRgba;
uniform float uRimThickness;

// sampling offsets to use when retrieving samples to feed
// into the kernel
vec2 offsets[9] = vec2[](
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

// simple edge-detection kernel
const float kernel[9] = float[](
    1.0,  1.0, 1.0,
    1.0, -8.0, 1.0,
    1.0,  1.0, 1.0
);

void main(void) {
    float rimStrength = 0.0;
    for (int i = 0; i < 9; ++i) {
        vec2 offset = uRimThickness * offsets[i];
        vec2 coord = TexCoord + offset;

        rimStrength += kernel[i] * texture(uSampler0, coord).b;
    }

    // the kernel:
    //
    // - produces positive strength for fragments on the outer rim
    // - produces negative strength for fragments on inner rim

    // rimStrength = abs(rimStrength);  // if you want inner edge, but it's buggy
    rimStrength = clamp(rimStrength, 0.0, 1.0);

    // alpha-over the compose the bg (sampler 1) behind the rims (sampler 0)
    FragColor = rimStrength * uRimRgba;
}
