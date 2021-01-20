#version 330 core

// edge detect: sample a texture with edge detection
//
// this fragment shader samples the incoming texture with a very specific alg. that
// detects edges in the texture, where edges are defined very specifically (TODO:
// a wiser programmer would be more specific about this BS)

in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;

uniform vec4 uBackgroundColor;
uniform sampler2DMS uSamplerSceneColors;
uniform sampler2DMS uSamplerSelectionEdges;
uniform int uNumMultisamples;

const float offset = 2.0;
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

const float kernel[9] = float[](
    1.0,  1.0, 1.0,
    1.0, -8.0, 1.0,
    1.0,  1.0, 1.0
);

void main(void) {
    // ignoring MSXAA for now
    // average 0-15 if you want smoothing
    // vec4 texel = texelFetch(uSampler0, ivec2(gl_FragCoord.xy), 0);

    // perform MSXAA here, so that we don't need auxiliary buffers
    vec4 sceneColor = vec4(0.0);
    for (int i = 0; i < uNumMultisamples; ++i) {
        sceneColor += texelFetch(uSamplerSceneColors, ivec2(gl_FragCoord.xy), i);
    }
    sceneColor /= uNumMultisamples;

    // compute rim highlights
    float rimStrength = 0.0;
    for(int i = 0; i < 9; i++) {
        float ms = 0.0;
        for (int j = 0; j < uNumMultisamples; ++j) {
            ms += kernel[i] * texelFetch(uSamplerSelectionEdges, ivec2(gl_FragCoord.xy + offsets[i]), j).a;
        }
        rimStrength += ms/uNumMultisamples;
    }

    // only want outer rim
    rimStrength = max(rimStrength, 0.0);

    // alpha-over the scene on top of the solid background color
    vec4 color = sceneColor.a * sceneColor + (1.0 - sceneColor.a) * uBackgroundColor;
    color = rimStrength * vec4(1.0, 0.0, 0.0, 1.0) + (1.0 - rimStrength) * color;

    FragColor = color;
}
