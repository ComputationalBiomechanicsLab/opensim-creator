#version 330 core

// passthrough frag shader: rendering does not use textures and we're
// using a "good enough" Gouraud shader (rather than a Phong shader,
// which is per-fragment).

in vec4 frag_color;
out vec4 color;

void main() {
    color = frag_color;
}
