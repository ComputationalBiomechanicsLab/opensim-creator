#version 330 core

uniform bool uIsTextured = false;
uniform sampler2D uSampler0;

in vec4 GouraudBrightness;
in vec4 Rgba0;
in vec4 Rgba1;
in vec2 TexCoord;

layout (location = 0) out vec4 Color0Out;
layout (location = 1) out vec4 Color1Out;

void main() {
    // write shaded geometry color
    if (uIsTextured) {
        Color0Out = texture(uSampler0, TexCoord);
    } else {
        Color0Out = GouraudBrightness * Rgba0;
    }

    // write passthrough colors
    Color1Out = Rgba1;
}
