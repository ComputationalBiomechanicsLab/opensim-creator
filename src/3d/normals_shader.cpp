#include "normals_shader.hpp"

#include "src/app.hpp"
#include "src/utils/helpers.hpp"

using namespace osc;

static std::string slurp(char const* p) {
    return slurp_into_string(App::resource(p));
}

osc::Normals_shader::Normals_shader() :
    program{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(slurp("shaders/draw_normals.vert")),
        gl::CompileFromSource<gl::Fragment_shader>(slurp("shaders/draw_normals.frag")),
        gl::CompileFromSource<gl::Geometry_shader>(slurp("shaders/draw_normals.geom")))},

    aPos{0},
    aNormal{1},

    uModelMat{gl::GetUniformLocation(program, "uModelMat")},
    uViewMat{gl::GetUniformLocation(program, "uViewMat")},
    uProjMat{gl::GetUniformLocation(program, "uProjMat")},
    uNormalMat{gl::GetUniformLocation(program, "uNormalMat")} {
}
