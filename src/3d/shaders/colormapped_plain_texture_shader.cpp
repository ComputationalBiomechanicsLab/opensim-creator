#include "colormapped_plain_texture_shader.hpp"

static char const g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uMVP;

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 texCoord;

    void main(void) {
        gl_Position = uMVP * vec4(aPos, 1.0f);
        texCoord = aTexCoord;
    }
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    in vec2 texCoord;

    layout (location = 0) out vec4 fragColor;

    uniform sampler2D uSampler0;
    uniform mat4 uSamplerMultiplier = mat4(1.0);

    void main(void) {
        fragColor = uSamplerMultiplier * texture(uSampler0, texCoord);
    }
)";

osc::Colormapped_plain_texture_shader::Colormapped_plain_texture_shader() :
    p{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(g_VertexShader),
        gl::CompileFromSource<gl::Fragment_shader>(g_FragmentShader))},

    aPos{0},
    aTexCoord{1},

    uMVP{gl::GetUniformLocation(p, "uMVP")},
    uSampler0{gl::GetUniformLocation(p, "uSampler0")},
    uSamplerMultiplier{gl::GetUniformLocation(p, "uSamplerMultiplier")} {
}
