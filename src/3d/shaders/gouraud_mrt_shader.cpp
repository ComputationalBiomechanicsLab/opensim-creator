#include "gouraud_mrt_shader.hpp"

static char g_VertexShader[] = R"(
    #version 330 core

    // gouraud_shader:
    //
    // performs lighting calculations per vertex (Gouraud shading), rather
    // than per frag ((Blinn-)Phong shading)

    uniform mat4 uProjMat;
    uniform mat4 uViewMat;
    uniform vec3 uLightDir;
    uniform vec3 uLightColor;
    uniform vec3 uViewPos;

    layout (location = 0) in vec3 aLocation;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoord;
    layout (location = 3) in mat4x3 aModelMat;
    layout (location = 7) in mat3 aNormalMat;
    layout (location = 10) in vec4 aRgba0;
    layout (location = 11) in float aRimIntensity;

    out vec4 GouraudBrightness;
    out vec4 Rgba0;
    out float RimIntensity;
    out vec2 TexCoord;

    const float ambientStrength = 0.5f;
    const float diffuseStrength = 0.3f;
    const float specularStrength = 0.1f;
    const float shininess = 32;

    void main() {
        mat4 modelMat = mat4(vec4(aModelMat[0], 0), vec4(aModelMat[1], 0), vec4(aModelMat[2], 0), vec4(aModelMat[3], 1));

        gl_Position = uProjMat * uViewMat * modelMat * vec4(aLocation, 1.0);

        vec3 normalDir = normalize(aNormalMat * aNormal);
        vec3 fragPos = vec3(modelMat * vec4(aLocation, 1.0));
        vec3 frag2viewDir = normalize(uViewPos - fragPos);
        vec3 frag2lightDir = normalize(-uLightDir);  // light dir is in the opposite direction

        vec3 ambientComponent = ambientStrength * uLightColor;

        float diffuseAmount = max(dot(normalDir, frag2lightDir), 0.0);
        vec3 diffuseComponent = diffuseStrength * diffuseAmount * uLightColor;

        vec3 halfwayDir = normalize(frag2lightDir + frag2viewDir);
        float specularAmmount = pow(max(dot(normalDir, halfwayDir), 0.0), shininess);
        vec3 specularComponent = specularStrength * specularAmmount * uLightColor;

        vec3 lightStrength = ambientComponent + diffuseComponent + specularComponent;

        GouraudBrightness = vec4(uLightColor * lightStrength, 1.0);
        Rgba0 = aRgba0;
        RimIntensity = aRimIntensity;
        TexCoord = aTexCoord;
    }
)";

static char const g_FragmentShader[] = R"(
    #version 330 core

    uniform bool uIsTextured = false;
    uniform sampler2D uSampler0;

    in vec4 GouraudBrightness;
    in vec4 Rgba0;
    in float RimIntensity;
    in vec2 TexCoord;

    layout (location = 0) out vec4 Color0Out;
    layout (location = 1) out float Color1Out;

    void main() {
        // write shaded geometry color
        vec4 color = uIsTextured ? texture(uSampler0, TexCoord) : Rgba0;
        color *= GouraudBrightness;
        Color0Out = color;
        Color1Out = RimIntensity;
    }
)";

osc::Gouraud_mrt_shader::Gouraud_mrt_shader() :
    program{gl::CreateProgramFrom(
        gl::CompileFromSource<gl::Vertex_shader>(g_VertexShader),
        gl::CompileFromSource<gl::Fragment_shader>(g_FragmentShader))},

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
    uSampler0{gl::GetUniformLocation(program, "uSampler0")} {
}
