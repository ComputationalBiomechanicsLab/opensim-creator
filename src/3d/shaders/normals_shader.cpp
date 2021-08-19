#include "normals_shader.hpp"

static char const g_VertexShader[] = R"(
    #version 330 core

    // draw_normals: program that draws mesh normals
    //
    // This vertex shader just passes each vertex/normal to the geometry shader, which
    // then uses that information to draw lines for each normal.

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    out VS_OUT {
        vec3 normal;
    } vs_out;

    void main() {
        gl_Position = vec4(aPos, 1.0f);
        vs_out.normal = aNormal;
    }
)";

static char const g_GeomShader[] = R"(
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
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    // draw_normals: program that draws mesh normals
    //
    // this frag shader doesn't do much: just color each line emitted by the geometry shader
    // so that the viewers can "see" normals

    out vec4 FragColor;

    void main() {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

osc::Normals_shader::Normals_shader() :
    program{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(g_VertexShader),
        gl::CompileFromSource<gl::Fragment_shader>(g_FragmentShader),
        gl::CompileFromSource<gl::Geometry_shader>(g_GeomShader))},

    uModelMat{gl::GetUniformLocation(program, "uModelMat")},
    uViewMat{gl::GetUniformLocation(program, "uViewMat")},
    uProjMat{gl::GetUniformLocation(program, "uProjMat")},
    uNormalMat{gl::GetUniformLocation(program, "uNormalMat")} {
}
