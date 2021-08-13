#include "colormapped_plain_texture_shader.hpp"

#include "src/app.hpp"
#include "src/utils/helpers.hpp"

using namespace osc;

static std::string slurp(char const* p) {
    return slurp_into_string(App::resource(p));
}

osc::Colormapped_plain_texture_shader::Colormapped_plain_texture_shader() :
    p{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(slurp("shaders/colormapped_plain_texture.vert")),
        gl::CompileFromSource<gl::Fragment_shader>(slurp("shaders/colormapped_plain_texture.frag")))},

    aPos{0},
    aTexCoord{1},

    uMVP{gl::GetUniformLocation(p, "uMVP")},
    uSampler0{gl::GetUniformLocation(p, "uSampler0")},
    uSamplerMultiplier{gl::GetUniformLocation(p, "uSamplerMultiplier")} {
}
