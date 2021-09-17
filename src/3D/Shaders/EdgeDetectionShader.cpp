#include "EdgeDetectionShader.hpp"

static char g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uMVP;

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main(void) {
        gl_Position = uMVP * vec4(aPos, 1.0f);
        TexCoord = aTexCoord;
    }
)";

static char g_FragmentShader[] = R"(
    #version 330 core

    uniform sampler2D uSampler0;
    uniform vec4 uRimRgba;
    uniform float uRimThickness;

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

    // simple edge-detection kernel
    const float kernel[9] = float[](
        1.0,  1.0, 1.0,
        1.0, -8.0, 1.0,
        1.0,  1.0, 1.0
    );

    void main(void) {

        float rimStrength = 0.0;
        for (int i = 0; i < 9; ++i) {
            vec2 offset = uRimThickness * offsets[i];
            vec2 coord = TexCoord + offset;

            rimStrength += kernel[i] * texture(uSampler0, coord).r;
        }

        // rimStrength = abs(rimStrength);  // for inner edges
        rimStrength = clamp(rimStrength, 0.0, 1.0);

        FragColor = rimStrength * uRimRgba;
    }
)";

osc::EdgeDetectionShader::EdgeDetectionShader() :
    program{gl::CreateProgramFrom(
          gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
          gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader))},

    uMVP{gl::GetUniformLocation(program, "uMVP")},
    uSampler0{gl::GetUniformLocation(program, "uSampler0")},
    uRimRgba{gl::GetUniformLocation(program, "uRimRgba")},
    uRimThickness{gl::GetUniformLocation(program, "uRimThickness")} {
}
