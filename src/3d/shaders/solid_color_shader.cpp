#include "solid_color_shader.hpp"

static char const g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform mat4 uModelMat;

    layout (location = 0) in vec3 aPos;

    void main() {
        gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);
    }
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    uniform vec4 uColor;

    out vec4 FragColor;

    void main() {
        FragColor = uColor;
    }
)";

osc::Solid_color_shader::Solid_color_shader() :
    prog{gl::CreateProgramFrom(
             gl::CompileFromSource<gl::Vertex_shader>(g_VertexShader),
             gl::CompileFromSource<gl::Fragment_shader>(g_FragmentShader))},
    aPos{0},
    uModel{gl::GetUniformLocation(prog, "uModelMat")},
    uView{gl::GetUniformLocation(prog, "uViewMat")},
    uProjection{gl::GetUniformLocation(prog, "uProjMat")},
    uColor{gl::GetUniformLocation(prog, "uColor")} {
}
