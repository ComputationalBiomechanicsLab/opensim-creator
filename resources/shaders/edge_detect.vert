#version 330 core

// edge detect: sample a texture with edge detection
//
// this vertex shader just passes through to the fragment shader, which does the
// actual work (it's assumed that the caller will render a quad that is texture
// mapped)

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uModelMat;
uniform mat4 uViewMat;
uniform mat4 uProjMat;

out vec2 TexCoord;

void main(void) {
    gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0f);
    TexCoord = aTexCoord;
}
