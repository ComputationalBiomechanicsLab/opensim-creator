#version 330 core

// skip msxaa blitter: texture sampler that only samples the first sample from
// a multisampled source
//
// useful if you need to guarantee that no sampled blending is happening to the
// source texture
//
// this vert shader is just a passthrough shader: magic happens in frag shader

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
