#version 330 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

const float MAGNITUDE = 0.01f;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;
uniform mat4 normalMat;

void GenerateLine(int index) {
    // emit original vertex as-is
    gl_Position = projMat * viewMat * modelMat * gl_in[index].gl_Position;
    EmitVertex();

    mat4 normalMatrix = normalMat;
    vec4 normalVec = normalize(projMat * viewMat * normalMatrix * vec4(gs_in[index].normal, 0.0f));
    normalVec *= MAGNITUDE;
    gl_Position = (projMat * viewMat * modelMat * gl_in[index].gl_Position) + normalVec;
    EmitVertex();

    // which results in a 2-vertex line primitive
    EndPrimitive();
}

void main() {
    GenerateLine(0); // first vertex normal
    GenerateLine(1); // second vertex normal
    GenerateLine(2); // third vertex normal
}
