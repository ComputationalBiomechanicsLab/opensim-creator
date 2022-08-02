#include "InstancedGouraudColorShader.hpp"

#include "src/Graphics/Gl.hpp"

static char g_VertexShader[] =
R"(
    #version 330 core

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform vec3 uLightDir;
    uniform vec3 uLightColor;
    uniform vec3 uViewPos;

    layout (location = 0) in vec3 aPos;
    layout (location = 2) in vec3 aNormal;

    layout (location = 6) in mat4x3 aModelMat;
    layout (location = 10) in mat3 aNormalMat;
    layout (location = 13) in vec4 aRgba0;

    out vec4 GouraudBrightness;
    out vec4 Rgba0;

    const float ambientStrength = 0.5f;
    const float diffuseStrength = 0.5f;
    const float specularStrength = 0.7f;
    const float shininess = 32;

    void main()
    {
        mat4 modelMat = mat4(vec4(aModelMat[0], 0), vec4(aModelMat[1], 0), vec4(aModelMat[2], 0), vec4(aModelMat[3], 1));

        gl_Position = uProjMat * uViewMat * modelMat * vec4(aPos, 1.0);

        vec3 normalDir = normalize(aNormalMat * aNormal);
        vec3 fragPos = vec3(modelMat * vec4(aPos, 1.0));
        vec3 frag2viewDir = normalize(uViewPos - fragPos);
        vec3 frag2lightDir = normalize(-uLightDir);  // light dir is in the opposite direction

        vec3 ambientComponent = ambientStrength * uLightColor;

        float diffuseAmount = abs(dot(normalDir, frag2lightDir));
        vec3 diffuseComponent = diffuseStrength * diffuseAmount * uLightColor;

        vec3 halfwayDir = normalize(frag2lightDir + frag2viewDir);
        float specularAmmount = pow(abs(dot(normalDir, halfwayDir)), shininess);
        vec3 specularComponent = specularStrength * specularAmmount * uLightColor;

        vec3 lightStrength = ambientComponent + diffuseComponent + specularComponent;

        GouraudBrightness = vec4(uLightColor * lightStrength, 1.0);
        Rgba0 = aRgba0;
    }
)";

static char const g_FragmentShader[] =
R"(
    #version 330 core

    in vec4 GouraudBrightness;
    in vec4 Rgba0;

    out vec4 ColorOut;

    void main()
    {
        ColorOut = GouraudBrightness * Rgba0;
    }
)";

osc::InstancedGouraudColorShader::InstancedGouraudColorShader() :
    program{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::VertexShader>(g_VertexShader),
        gl::CompileFromSource<gl::FragmentShader>(g_FragmentShader))},

    uProjMat{gl::GetUniformLocation(program, "uProjMat")},
    uViewMat{gl::GetUniformLocation(program, "uViewMat")},
    uLightDir{gl::GetUniformLocation(program, "uLightDir")},
    uLightColor{gl::GetUniformLocation(program, "uLightColor")},
    uViewPos{gl::GetUniformLocation(program, "uViewPos")} {
}
