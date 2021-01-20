#version 330 core

// draw_normals: program that draws mesh normals
//
// This geometry shader generates a line strip for each normal it is given. The downstream
// fragment shader then fills in each line, so that the viewer can see normals as lines
// poking out of the mesh

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

uniform mat4 uModelMat;
uniform mat4 uViewMat;
uniform mat4 uProjMat;
uniform mat4 uNormalMat;

const float NORMAL_LINE_LEN = 0.01f;

void GenerateLine(int index) {
    vec4 origVertexPos = uProjMat * uViewMat * uModelMat * gl_in[index].gl_Position;

    // emit original vertex in original position
    gl_Position = origVertexPos;
    EmitVertex();

    // calculate normal vector *direction*
    vec4 normalVec = normalize(uProjMat * uViewMat * uNormalMat * vec4(gs_in[index].normal, 0.0f));

    // then scale the direction vector to some fixed length (of line)
    normalVec *= NORMAL_LINE_LEN;

    // emit another vertex (the line "tip")
    gl_Position = origVertexPos + normalVec;
    EmitVertex();

    // emit line primitve
    EndPrimitive();
}

void main() {
    GenerateLine(0); // first vertex normal
    GenerateLine(1); // second vertex normal
    GenerateLine(2); // third vertex normal
}
