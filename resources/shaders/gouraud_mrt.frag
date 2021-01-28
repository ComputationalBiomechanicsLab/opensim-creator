#version 330 core

// passthrough frag shader: rendering does not use textures and we're
// using a "good enough" Gouraud shader (rather than a Phong shader,
// which is per-fragment).

in vec4 FragColor;

layout (location = 0) out vec4 Color0Out;
layout (location = 1) out vec4 Color1Out;

uniform vec4 uRgba1;

void main() {
    // write shaded geometry color
    Color0Out = FragColor;

    // write passthrough colors
    Color1Out = uRgba1;
}
