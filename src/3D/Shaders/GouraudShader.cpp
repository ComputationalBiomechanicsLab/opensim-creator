#include "GouraudShader.hpp"

static char g_VertexShader[] = R"(
    #version 330 core

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform mat4 uModelMat;
    uniform mat3 uNormalMat;
    uniform vec3 uLightDir;
    uniform vec3 uLightColor;
    uniform vec3 uViewPos;

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    layout (location = 2) in vec3 aNormal;

    out vec4 GouraudBrightness;
    out vec2 TexCoord;

    const float ambientStrength = 0.7f;
    const float diffuseStrength = 0.3f;
    const float specularStrength = 0.1f;
    const float shininess = 32;

    void main() {
        gl_Position = uProjMat * uViewMat * uModelMat * vec4(aPos, 1.0);

        vec3 normalDir = normalize(uNormalMat * aNormal);
        vec3 fragPos = vec3(uModelMat * vec4(aPos, 1.0));
        vec3 frag2viewDir = normalize(uViewPos - fragPos);
        vec3 frag2lightDir = normalize(-uLightDir);  // light dir is in the opposite direction
        vec3 halfwayDir = (frag2lightDir + frag2viewDir)/2.0;

        float ambientAmt = ambientStrength;
        float diffuseAmt = diffuseStrength * max(dot(normalDir, frag2lightDir), 0.0);
        float specularAmt = specularStrength * pow(max(dot(normalDir, halfwayDir), 0.0), shininess);

        float lightAmt = clamp(ambientAmt + diffuseAmt + specularAmt, 0.0, 1.0);

        GouraudBrightness = vec4(lightAmt * uLightColor, 1.0);
        TexCoord = aTexCoord;
    }
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    uniform bool uIsTextured = false;
    uniform sampler2D uSampler0;
    uniform vec4 uDiffuseColor = vec4(1.0, 1.0, 1.0, 1.0);

    in vec4 GouraudBrightness;
    in vec2 TexCoord;

    out vec4 Color0Out;

    void main() {
        vec4 color = uIsTextured ? uDiffuseColor * texture(uSampler0, TexCoord) : uDiffuseColor;
        color *= GouraudBrightness;

        Color0Out = color;
    }
)";

osc::GouraudShader::GouraudShader() :
    program{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
        gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader))},

    uProjMat{gl::GetUniformLocation(program, "uProjMat")},
    uViewMat{gl::GetUniformLocation(program, "uViewMat")},
    uModelMat{gl::GetUniformLocation(program, "uModelMat")},
    uNormalMat{gl::GetUniformLocation(program, "uNormalMat")},
    uDiffuseColor{gl::GetUniformLocation(program, "uDiffuseColor")},
    uLightDir{gl::GetUniformLocation(program, "uLightDir")},
    uLightColor{gl::GetUniformLocation(program, "uLightColor")},
    uViewPos{gl::GetUniformLocation(program, "uViewPos")},
    uIsTextured{gl::GetUniformLocation(program, "uIsTextured")},
    uSampler0{gl::GetUniformLocation(program, "uSampler0")} {
}


