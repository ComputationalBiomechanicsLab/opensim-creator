#version 330 core

// skip msxaa blitter: texture sampler that only samples the first sample from
// a multisampled source
//
// useful if you need to guarantee that no sampled blending is happening to the
// source texture

in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;

uniform sampler2DMS uSampler0;

void main(void) {
    FragColor = texelFetch(uSampler0, ivec2(gl_FragCoord.xy), 0);
}
