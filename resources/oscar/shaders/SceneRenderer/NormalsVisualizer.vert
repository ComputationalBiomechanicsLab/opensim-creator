#version 330 core

// draw_normals: program that draws mesh normals
//
// This vertex shader just passes each vertex/normal to the geometry shader, which
// then uses that information to draw lines for each normal.

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;

out VS_OUT {
    vec3 normal;
} vs_out;

void main()
{
    gl_Position = vec4(aPos, 1.0f);
    vs_out.normal = aNormal;
}
