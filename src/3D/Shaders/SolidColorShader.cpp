#include "SolidColorShader.hpp"

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

osc::SolidColorShader::SolidColorShader() :
    program{gl::CreateProgramFrom(
             gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
             gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader))},

    uModel{gl::GetUniformLocation(program, "uModelMat")},
    uView{gl::GetUniformLocation(program, "uViewMat")},
    uProjection{gl::GetUniformLocation(program, "uProjMat")},
    uColor{gl::GetUniformLocation(program, "uColor")} {
}
