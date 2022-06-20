#include "EdgeDetectionShader.hpp"

#include "src/Graphics/Gl.hpp"

static char g_VertexShader[] =
R"(
    #version 330 core

    uniform mat4 uMVP;

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main(void)
    {
        gl_Position = uMVP * vec4(aPos, 1.0f);
        TexCoord = aTexCoord;
    }
)";

static char g_FragmentShader[] =
R"(
    #version 330 core

    uniform sampler2D uSampler0;
    uniform vec4 uRimRgba;
    uniform vec2 uRimThickness;

    in vec2 TexCoord;

    out vec4 FragColor;

    // sampling offsets to use when retrieving samples to feed
    // into the kernel
    const vec2 offsets[9] = vec2[](
        vec2(-1.0f,  1.0f), // top-left
        vec2( 0.0f,  1.0f), // top-center
        vec2( 1.0f,  1.0f), // top-right
        vec2(-1.0f,  0.0f), // center-left
        vec2( 0.0f,  0.0f), // center-center
        vec2( 1.0f,  0.0f), // center-right
        vec2(-1.0f, -1.0f), // bottom-left
        vec2( 0.0f, -1.0f), // bottom-center
        vec2( 1.0f, -1.0f)  // bottom-right
    );

    // https://computergraphics.stackexchange.com/questions/2450/opengl-detection-of-edges
    const float xkern[9] = float[](
        +1.0, 0.0, -1.0,
        +2.0, 0.0, -2.0,
        +1.0, 0.0, -1.0
    );

    const float ykern[9] = float[](
        +1.0, +2.0, +1.0,
         0.0,  0.0,  0.0,
        -1.0, -2.0, -1.0
    );

    void main(void)
    {
        float rimX = 0.0;
        float rimY = 0.0;
        for (int i = 0; i < xkern.length(); ++i) {
            vec2 offset = uRimThickness * offsets[i];
            vec2 coord = TexCoord + offset;

            float v = texture(uSampler0, coord).r;
            float x = xkern[i] * v;
            float y = ykern[i] * v;

            rimX += x;
            rimY += y;
        }

        float rimStrength = sqrt(rimX*rimX + rimY*rimY) / 3.0f;

        // rimStrength = abs(rimStrength);  // for inner edges

        FragColor = vec4(uRimRgba.rgb, rimStrength * uRimRgba.a);
    }
)";

osc::EdgeDetectionShader::EdgeDetectionShader() :
    program{gl::CreateProgramFrom(
          gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
          gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader))},

    uMVP{gl::GetUniformLocation(program, "uMVP")},
    uSampler0{gl::GetUniformLocation(program, "uSampler0")},
    uRimRgba{gl::GetUniformLocation(program, "uRimRgba")},
    uRimThickness{gl::GetUniformLocation(program, "uRimThickness")}
{
}
