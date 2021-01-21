#version 330 core

// edge detect: sample a texture with edge detection

in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;

uniform vec4 uBackgroundColor;
uniform sampler2DMS uSamplerSceneColors;
uniform sampler2DMS uSamplerSelectionEdges;
uniform int uNumMultisamples;

// sampling offsets to use when retrieving samples to feed
// into the kernel
const float offset = 1.5;
vec2 offsets[9] = vec2[](
    vec2(-offset,  offset), // top-left
    vec2( 0.0f,    offset), // top-center
    vec2( offset,  offset), // top-right
    vec2(-offset,  0.0f),   // center-left
    vec2( 0.0f,    0.0f),   // center-center
    vec2( offset,  0.0f),   // center-right
    vec2(-offset, -offset), // bottom-left
    vec2( 0.0f,   -offset), // bottom-center
    vec2( offset, -offset)  // bottom-right
);

// simple edge-detection kernel
const float kernel[9] = float[](
    1.0,  1.0, 1.0,
    1.0, -8.0, 1.0,
    1.0,  1.0, 1.0
);

void main(void) {

    // compute scene color by resolving its MSXAA samples
    vec4 sceneColor = vec4(0.0);
    for (int i = 0; i < uNumMultisamples; ++i) {
        // TODO: don't use gl_FragCoord
        sceneColor += texelFetch(uSamplerSceneColors, ivec2(gl_FragCoord.xy), i);
    }
    sceneColor /= uNumMultisamples;

    // compute rim highlights by resolving each selection selection texel with
    // MSXAA
    float rimStrength = 0.0;
    for (int i = 0; i < 9; ++i) {

        // TODO: don't use gl_FragCoord
        ivec2 offset = ivec2(gl_FragCoord.xy + offsets[i]);

        float val = 0.0;
        for (int j = 0; j < uNumMultisamples; ++j) {
            val += texelFetch(uSamplerSelectionEdges, offset, j).a;
        }
        val /= uNumMultisamples;

        rimStrength += kernel[i] * val;
    }

    // the kernel:
    //
    // - produces positive strength for fragments on the outer rim
    // - produces negative strength for fragments on inner rim

    // rimStrength = abs(rimStrength);  // if you want inner edge, but it's buggy

    rimStrength = clamp(rimStrength, 0.0, 1.0);

    // alpha-over the scene on top of the solid background color
    vec4 color = sceneColor.a * sceneColor + (1.0 - sceneColor.a) * uBackgroundColor;
    color = rimStrength * vec4(1.0, 0.3, 0.0, 1.0) + (1.0 - rimStrength) * color;

    FragColor = color;
}
