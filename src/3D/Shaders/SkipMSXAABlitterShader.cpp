#include "SkipMSXAABlitterShader.hpp"

static char const g_VertexShader[] = R"(
    #version 330 core

    // skip msxaa blitter: texture sampler that only samples the first sample from
    // a multisampled source
    //
    // useful if you need to guarantee that no sampled blending is happening to the
    // source texture
    //
    // this vert shader is just a passthrough shader: magic happens in frag shader

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    uniform mat4 uModelMat;
    uniform mat4 uViewMat;
    uniform mat4 uProjMat;

    out vec2 TexCoord;

    void main(void) {
        gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0f);
        TexCoord = aTexCoord;
    }
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    // skip msxaa blitter: texture sampler that only samples the first sample from
    // a multisampled source
    //
    // useful if you need to guarantee that no sampled blending is happening to the
    // source texture

    in vec2 TexCoord;
    layout (location = 0) out vec4 FragColor;

    uniform sampler2DMS uSampler0;

    void main(void) {
        FragColor = texelFetch(uSampler0, ivec2(gl_FragCoord.xy), 0);
    }
)";

osc::SkipMSXAABlitterShader::SkipMSXAABlitterShader() :
    program{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
        gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader))},

    uModelMat{gl::GetUniformLocation(program, "uModelMat")},
    uViewMat{gl::GetUniformLocation(program, "uViewMat")},
    uProjMat{gl::GetUniformLocation(program, "uProjMat")},
    uSampler0{gl::GetUniformLocation(program, "uSampler0")} {
}
