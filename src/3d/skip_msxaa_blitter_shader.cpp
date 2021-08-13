#include "skip_msxaa_blitter_shader.hpp"

#include "src/app.hpp"
#include "src/utils/helpers.hpp"

using namespace osc;

static std::string slurp(char const* p) {
    return slurp_into_string(App::resource(p));
}

osc::Skip_msxaa_blitter_shader::Skip_msxaa_blitter_shader() :
    p{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(slurp("shaders/skip_msxaa_blitter.vert")),
        gl::CompileFromSource<gl::Fragment_shader>(slurp("shaders/skip_msxaa_blitter.frag")))},

    aPos{0},
    aTexCoord{1},

    uModelMat{gl::GetUniformLocation(p, "uModelMat")},
    uViewMat{gl::GetUniformLocation(p, "uViewMat")},
    uProjMat{gl::GetUniformLocation(p, "uProjMat")},
    uSampler0{gl::GetUniformLocation(p, "uSampler0")} {
}
