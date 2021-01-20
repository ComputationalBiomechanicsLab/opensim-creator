#version 330 core

// passthrough frag shader: rendering does not use textures and we're
// using a "good enough" Gouraud shader (rather than a Phong shader,
// which is per-fragment).

in vec4 FragColor;
out vec4 FragColorOut;

void main() {
    FragColorOut = FragColor;
}
