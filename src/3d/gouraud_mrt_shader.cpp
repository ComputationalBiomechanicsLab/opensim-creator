#include "gouraud_mrt_shader.hpp"

#include "src/app.hpp"
#include "src/utils/helpers.hpp"

using namespace osc;

static std::string slurp(char const* p) {
    return slurp_into_string(App::resource(p));
}

osc::Gouraud_mrt_shader::Gouraud_mrt_shader() :
    program{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(slurp("shaders/gouraud_mrt.vert")),
        gl::CompileFromSource<gl::Fragment_shader>(slurp("shaders/gouraud_mrt.frag")))},

    aLocation{0},
    aNormal{1},
    aTexCoord{2},

    aModelMat{3},
    aNormalMat{7},
    aRgba0{10},
    aRimIntensity{11},

    uProjMat{gl::GetUniformLocation(program, "uProjMat")},
    uViewMat{gl::GetUniformLocation(program, "uViewMat")},
    uLightDir{gl::GetUniformLocation(program, "uLightDir")},
    uLightColor{gl::GetUniformLocation(program, "uLightColor")},
    uViewPos{gl::GetUniformLocation(program, "uViewPos")},
    uIsTextured{gl::GetUniformLocation(program, "uIsTextured")},
    uIsShaded{gl::GetUniformLocation(program, "uIsShaded")},
    uSampler0{gl::GetUniformLocation(program, "uSampler0")},
    uSkipVP{gl::GetUniformLocation(program, "uSkipVP")} {
}
