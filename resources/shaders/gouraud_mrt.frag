#version 330 core

// passthrough frag shader: rendering does not use textures and we're
// using a "good enough" Gouraud shader (rather than a Phong shader,
// which is per-fragment).

in vec4 FragColor;

layout (location = 0) out vec4 FragColorOut;
layout (location = 1) out vec4 Rgba2Out;

uniform vec4 uRgba2;

void main() {
    FragColorOut = FragColor;
    Rgba2Out = uRgba2;
}
