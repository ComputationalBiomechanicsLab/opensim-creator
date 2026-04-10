#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 uShadowMatrices[6];

out vec3 WorldPos;

void main()
{
    for (int face = 0; face < 6; ++face)
    {
        for (int i = 0; i < 3; ++i)  // for each triangle's vertices
        {
            gl_Layer = face;
            gl_Position = uShadowMatrices[face] * gl_in[i].gl_Position;
            WorldPos = vec3(gl_in[i].gl_Position);

            EmitVertex();
        }
        EndPrimitive();
    }
}
