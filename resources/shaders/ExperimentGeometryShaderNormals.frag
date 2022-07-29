#version 330 core

// draw_normals: program that draws mesh normals
//
// this frag shader doesn't do much: just color each line emitted by the geometry shader
// so that the viewers can "see" normals

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}