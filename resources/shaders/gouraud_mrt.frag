#version 330 core

in vec4 Color0in;
in vec4 Color1in;

layout (location = 0) out vec4 Color0Out;
layout (location = 1) out vec4 Color1Out;

void main() {
    // write shaded geometry color
    Color0Out = Color0in;

    // write passthrough colors
    Color1Out = Color1in;
}
