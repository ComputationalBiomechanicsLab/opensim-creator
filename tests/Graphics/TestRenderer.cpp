#include "src/Graphics/Camera.hpp"
#include "src/Graphics/CameraClearFlags.hpp"
#include "src/Graphics/CameraProjection.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/ColorSpace.hpp"
#include "src/Graphics/Cubemap.hpp"
#include "src/Graphics/DepthStencilFormat.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsContext.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/MaterialPropertyBlock.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/MeshTopology.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/RenderTextureDescriptor.hpp"
#include "src/Graphics/RenderTextureFormat.hpp"
#include "src/Graphics/RenderTextureReadWrite.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Graphics/TextureFormat.hpp"
#include "src/Graphics/TextureWrapMode.hpp"
#include "src/Graphics/TextureFilterMode.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/ShaderType.hpp"

#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>

static std::unique_ptr<osc::App> g_App;

class Renderer : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        g_App = std::make_unique<osc::App>();
    }

    static void TearDownTestSuite()
    {
        g_App.reset();
    }
};

static constexpr char const g_VertexShaderSrc[] =
R"(
    #version 330 core

    uniform mat4 uViewProjMat;
    uniform mat4 uLightSpaceMat;
    uniform vec3 uLightDir;
    uniform vec3 uViewPos;
    uniform float uDiffuseStrength = 0.85f;
    uniform float uSpecularStrength = 0.4f;
    uniform float uShininess = 8;

    layout (location = 0) in vec3 aPos;
    layout (location = 2) in vec3 aNormal;
    layout (location = 6) in mat4 aModelMat;
    layout (location = 10) in mat3 aNormalMat;

    out vec3 FragWorldPos;
    out vec4 FragLightSpacePos;
    out vec3 NormalWorldDir;
    out float NonAmbientBrightness;

    void main()
    {
        vec3 normalDir = normalize(aNormalMat * aNormal);
        vec3 fragPos = vec3(aModelMat * vec4(aPos, 1.0));
        vec3 frag2viewDir = normalize(uViewPos - fragPos);
        vec3 frag2lightDir = normalize(-uLightDir);  // light dir is in the opposite direction
        vec3 halfwayDir = 0.5 * (frag2lightDir + frag2viewDir);

        float diffuseAmt = uDiffuseStrength * abs(dot(normalDir, frag2lightDir));
        float specularAmt = uSpecularStrength * pow(abs(dot(normalDir, halfwayDir)), uShininess);

        vec4 worldPos = aModelMat * vec4(aPos, 1.0);

        FragWorldPos = vec3(aModelMat * vec4(aPos, 1.0));
        FragLightSpacePos = uLightSpaceMat * worldPos;
        NormalWorldDir = normalDir;
        NonAmbientBrightness = diffuseAmt + specularAmt;

        gl_Position = uViewProjMat * worldPos;
    }
)";

static constexpr char const g_FragmentShaderSrc[] =
R"(
    #version 330 core

    uniform bool uHasShadowMap = false;
    uniform vec3 uLightDir;
    uniform sampler2D uShadowMapTexture;
    uniform float uAmbientStrength = 0.15f;
    uniform vec3 uLightColor;
    uniform vec4 uDiffuseColor = vec4(1.0, 1.0, 1.0, 1.0);
    uniform float uNear;
    uniform float uFar;

    in vec3 FragWorldPos;
    in vec4 FragLightSpacePos;
    in vec3 NormalWorldDir;
    in float NonAmbientBrightness;

    out vec4 Color0Out;

    float CalculateShadowAmount()
    {
        // perspective divide
        vec3 projCoords = FragLightSpacePos.xyz / FragLightSpacePos.w;

        // map to [0, 1]
        projCoords = 0.5*projCoords + 0.5;

        // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
        float closestDepth = texture(uShadowMapTexture, projCoords.xy).r;

        // get depth of current fragment from light's perspective
        float currentDepth = projCoords.z;

        // calculate bias (based on depth map resolution and slope)
        float bias = max(0.025 * (1.0 - abs(dot(NormalWorldDir, uLightDir))), 0.0025);

        // check whether current frag pos is in shadow
        // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
        // PCF
        float shadow = 0.0;
        vec2 texelSize = 1.0 / textureSize(uShadowMapTexture, 0);
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(uShadowMapTexture, projCoords.xy + vec2(x, y) * texelSize).r;
                if (pcfDepth < 1.0)
                {
                    shadow += (currentDepth - bias) > pcfDepth  ? 1.0 : 0.0;
                }
            }
        }
        shadow /= 9.0;

        return shadow;
    }

    float LinearizeDepth(float depth)
    {
        // from: https://learnopengl.com/Advanced-OpenGL/Depth-testing
        //
        // only really works with perspective cameras: orthogonal cameras
        // don't need this unprojection math trick

        float z = depth * 2.0 - 1.0;
        return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
    }

    void main()
    {
        float shadowAmt = uHasShadowMap ? 0.5*CalculateShadowAmount() : 0.0f;
        float brightness = uAmbientStrength + ((1.0 - shadowAmt) * NonAmbientBrightness);
        Color0Out = vec4(brightness * uLightColor, 1.0) * uDiffuseColor;
        Color0Out.a *= 1.0 - (LinearizeDepth(gl_FragCoord.z) / uFar);  // fade into background at high distances
        Color0Out.a = clamp(Color0Out.a, 0.0, 1.0);
    }
)";

static constexpr char const g_VertexShaderWithArray[] =
R"(
    #version 330 core

    layout (location = 0) in vec3 aPos;

    void main()
    {
        gl_Position = vec4(aPos, 1.0);
    }
)";

static constexpr char const g_FragmentShaderWithArray[] =
R"(
    #version 330 core

    uniform vec4 uFragColor[3];

    out vec4 FragColor;

    void main()
    {
        FragColor = uFragColor[0];
    }
)";

// expected, based on the above shader code
static constexpr std::array<osc::CStringView, 14> g_ExpectedPropertyNames =
{
    "uViewProjMat",
    "uLightSpaceMat",
    "uLightDir",
    "uViewPos",
    "uDiffuseStrength",
    "uSpecularStrength",
    "uShininess",
    "uHasShadowMap",
    "uShadowMapTexture",
    "uAmbientStrength",
    "uLightColor",
    "uDiffuseColor",
    "uNear",
    "uFar",
};

static constexpr std::array<osc::ShaderType, 14> g_ExpectedPropertyTypes =
{
    osc::ShaderType::Mat4,
    osc::ShaderType::Mat4,
    osc::ShaderType::Vec3,
    osc::ShaderType::Vec3,
    osc::ShaderType::Float,
    osc::ShaderType::Float,
    osc::ShaderType::Float,
    osc::ShaderType::Bool,
    osc::ShaderType::Sampler2D,
    osc::ShaderType::Float,
    osc::ShaderType::Vec3,
    osc::ShaderType::Vec4,
    osc::ShaderType::Float,
    osc::ShaderType::Float,
};
static_assert(g_ExpectedPropertyNames.size() == g_ExpectedPropertyTypes.size());

static constexpr char const g_GeometryShaderVertSrc[] =
R"(
    #version 330 core

    // draw_normals: program that draws mesh normals
    //
    // This vertex shader just passes each vertex/normal to the geometry shader, which
    // then uses that information to draw lines for each normal.

    layout (location = 0) in vec3 aPos;
    layout (location = 2) in vec3 aNormal;

    out VS_OUT {
        vec3 normal;
    } vs_out;

    void main()
    {
        gl_Position = vec4(aPos, 1.0f);
        vs_out.normal = aNormal;
    }
)";

static constexpr char const g_GeometryShaderGeomSrc[] =
R"(
    #version 330 core

    // draw_normals: program that draws mesh normals
    //
    // This geometry shader generates a line strip for each normal it is given. The downstream
    // fragment shader then fills in each line, so that the viewer can see normals as lines
    // poking out of the mesh

    uniform mat4 uModelMat;
    uniform mat4 uViewProjMat;
    uniform mat4 uNormalMat;

    layout (triangles) in;
    in VS_OUT {
        vec3 normal;
    } gs_in[];

    layout (line_strip, max_vertices = 6) out;

    const float NORMAL_LINE_LEN = 0.01f;

    void GenerateLine(int index)
    {
        vec4 origVertexPos = uViewProjMat * uModelMat * gl_in[index].gl_Position;

        // emit original vertex in original position
        gl_Position = origVertexPos;
        EmitVertex();

        // calculate normal vector *direction*
        vec4 normalVec = normalize(uViewProjMat * uNormalMat * vec4(gs_in[index].normal, 0.0f));

        // then scale the direction vector to some fixed length (of line)
        normalVec *= NORMAL_LINE_LEN;

        // emit another vertex (the line "tip")
        gl_Position = origVertexPos + normalVec;
        EmitVertex();

        // emit line primitve
        EndPrimitive();
    }

    void main()
    {
        GenerateLine(0); // first vertex normal
        GenerateLine(1); // second vertex normal
        GenerateLine(2); // third vertex normal
    }
)";

static constexpr char const g_GeometryShaderFragSrc[] =
R"(
    #version 330 core

    // draw_normals: program that draws mesh normals
    //
    // this frag shader doesn't do much: just color each line emitted by the geometry shader
    // so that the viewers can "see" normals

    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

static std::default_random_engine& GetRngEngine()
{
    static std::default_random_engine e{};  // deterministic, because test failures due to RNG can suck
    return e;
}

static float GenerateFloat()
{
    return static_cast<float>(std::uniform_real_distribution{}(GetRngEngine()));
}

static int GenerateInt()
{
    return std::uniform_int_distribution{}(GetRngEngine());
}

static bool GenerateBool()
{
    return GenerateInt();
}

static osc::Color GenerateColor()
{
    return osc::Color{GenerateFloat(), GenerateFloat(), GenerateFloat(), GenerateFloat()};
}

static glm::vec2 GenerateVec2()
{
    return glm::vec2{GenerateFloat(), GenerateFloat()};
}

static glm::vec3 GenerateVec3()
{
    return glm::vec3{GenerateFloat(), GenerateFloat(), GenerateFloat()};
}

static glm::vec4 GenerateVec4()
{
    return glm::vec4{GenerateFloat(), GenerateFloat(), GenerateFloat(), GenerateFloat()};
}

static glm::mat3x3 GenerateMat3x3()
{
    return glm::mat3{GenerateVec3(), GenerateVec3(), GenerateVec3()};
}

static glm::mat4x4 GenerateMat4x4()
{
    return glm::mat4{GenerateVec4(), GenerateVec4(), GenerateVec4(), GenerateVec4()};
}

static glm::mat4x3 GenerateMat4x3()
{
    return glm::mat4x3{GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3()};
}

static osc::Texture2D GenerateTexture()
{
    std::vector<osc::Rgba32> pixels(4);

    return osc::Texture2D{{2, 2}, pixels, osc::ColorSpace::sRGB};
}

static osc::Material GenerateMaterial()
{
    osc::Shader shader{g_VertexShaderSrc, g_FragmentShaderSrc};
    return osc::Material{shader};
}

static std::vector<glm::vec3> GenerateTriangleVerts()
{
    std::vector<glm::vec3> rv;
    for (size_t i = 0; i < 30; ++i)
    {
        rv.push_back(GenerateVec3());
    }
    return rv;
}

static osc::RenderTexture GenerateRenderTexture()
{
    osc::RenderTextureDescriptor d{{2, 2}};
    return osc::RenderTexture{d};
}

template<typename T>
static bool SpansEqual(nonstd::span<T const> a, nonstd::span<T const> b)
{
    if (a.size() != b.size())
    {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i])
        {
            return false;
        }
    }

    return true;
}


// TESTS

TEST_F(Renderer, ShaderTypeCanStreamToString)
{
    std::stringstream ss;
    ss << osc::ShaderType::Bool;

    ASSERT_EQ(ss.str(), "Bool");
}

TEST_F(Renderer, ShaderTypeCanBeIteratedOverAndAllCanBeStreamed)
{
    for (int i = 0; i < static_cast<int>(osc::ShaderType::TOTAL); ++i)
    {
        // shouldn't crash - if it does then we've missed a case somewhere
        std::stringstream ss;
        ss << static_cast<osc::ShaderType>(i);
    }
}

TEST_F(Renderer, ShaderCanBeConstructedFromVertexAndFragmentShaderSource)
{
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
}

TEST_F(Renderer, ShaderCanBeConstructedFromVertexGeometryAndFragmentShaderSources)
{
    osc::Shader s{g_GeometryShaderVertSrc, g_GeometryShaderGeomSrc, g_GeometryShaderFragSrc};
}

TEST_F(Renderer, ShaderCanBeCopyConstructed)
{
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::Shader copy{s};
}

TEST_F(Renderer, ShaderCanBeMoveConstructed)
{
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::Shader copy{std::move(s)};
}

TEST_F(Renderer, ShaderCanBeCopyAssigned)
{
    osc::Shader s1{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::Shader s2{g_VertexShaderSrc, g_FragmentShaderSrc};

    s1 = s2;
}

TEST_F(Renderer, ShaderCanBeMoveAssigned)
{
    osc::Shader s1{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::Shader s2{g_VertexShaderSrc, g_FragmentShaderSrc};

    s1 = std::move(s2);
}

TEST_F(Renderer, ShaderThatIsCopyConstructedEqualsSrcShader)
{
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::Shader copy{s};

    ASSERT_EQ(s, copy);
}

TEST_F(Renderer, ShadersThatDifferCompareNotEqual)
{
    osc::Shader s1{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::Shader s2{g_VertexShaderSrc, g_FragmentShaderSrc};

    ASSERT_NE(s1, s2);
}

TEST_F(Renderer, ShaderCanBeWrittenToOutputStream)
{
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    std::stringstream ss;
    ss << s;  // shouldn't throw etc.

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(Renderer, ShaderOutputStreamContainsExpectedInfo)
{
    // this test is flakey, but is just ensuring that the string printout has enough information
    // to help debugging etc.

    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    std::stringstream ss;
    ss << s;
    std::string str{std::move(ss).str()};

    for (auto const& propName : g_ExpectedPropertyNames)
    {
        ASSERT_TRUE(osc::ContainsSubstring(str, std::string{propName}));
    }
}

TEST_F(Renderer, ShaderFindPropertyIndexCanFindAllExpectedProperties)
{
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    for (auto const& propName : g_ExpectedPropertyNames)
    {
        ASSERT_TRUE(s.findPropertyIndex(std::string{propName}));
    }
}
TEST_F(Renderer, ShaderHasExpectedNumberOfProperties)
{
    // (effectively, number of properties == number of uniforms)
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    ASSERT_EQ(s.getPropertyCount(), g_ExpectedPropertyNames.size());
}

TEST_F(Renderer, ShaderIteratingOverPropertyIndicesForNameReturnsValidPropertyName)
{
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    std::unordered_set<std::string> allPropNames;
    for (auto const& sv : g_ExpectedPropertyNames)
    {
        allPropNames.insert(std::string{sv});
    }
    std::unordered_set<std::string> returnedPropNames;

    for (size_t i = 0, len = s.getPropertyCount(); i < len; ++i)
    {
        returnedPropNames.insert(s.getPropertyName(i));
    }

    ASSERT_EQ(allPropNames, returnedPropNames);
}

TEST_F(Renderer, ShaderGetPropertyNameReturnsGivenPropertyName)
{
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    for (auto const& propName : g_ExpectedPropertyNames)
    {
        std::optional<size_t> idx = s.findPropertyIndex(std::string{propName});
        ASSERT_TRUE(idx);
        ASSERT_EQ(s.getPropertyName(*idx), propName);
    }
}

TEST_F(Renderer, ShaderGetPropertyNameStillWorksIfTheUniformIsAnArray)
{
    osc::Shader s{g_VertexShaderWithArray, g_FragmentShaderWithArray};
    ASSERT_FALSE(s.findPropertyIndex("uFragColor[0]").has_value()) << "shouldn't expose 'raw' name";
    ASSERT_TRUE(s.findPropertyIndex("uFragColor").has_value()) << "should work, because the backend should normalize array-like uniforms to the original name (not uFragColor[0])";
}

TEST_F(Renderer, ShaderGetPropertyTypeReturnsExpectedType)
{
    osc::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    for (size_t i = 0; i < g_ExpectedPropertyNames.size(); ++i)
    {
        static_assert(g_ExpectedPropertyNames.size() == g_ExpectedPropertyTypes.size());
        auto const& propName = g_ExpectedPropertyNames[i];
        osc::ShaderType expectedType = g_ExpectedPropertyTypes[i];

        std::optional<size_t> idx = s.findPropertyIndex(std::string{propName});
        ASSERT_TRUE(idx);
        ASSERT_EQ(s.getPropertyType(*idx), expectedType);
    }
}

TEST_F(Renderer, ShaderGetPropertyForCubemapReturnsExpectedType)
{
    // from: https://learnopengl.com/Advanced-OpenGL/Cubemaps
    char constexpr c_CubemapVertexShader[] = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

    char constexpr c_CubemapFragmentShader[] = R"(
#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, TexCoords);
}
)";

    osc::Shader shader{c_CubemapVertexShader, c_CubemapFragmentShader};
    std::optional<size_t> index = shader.findPropertyIndex("skybox");

    ASSERT_TRUE(index.has_value());
    ASSERT_EQ(shader.getPropertyType(*index), osc::ShaderType::SamplerCube);
}

TEST_F(Renderer, MaterialCanBeConstructed)
{
    GenerateMaterial();  // should compile and run fine
}

TEST_F(Renderer, MaterialCanBeCopyConstructed)
{
    osc::Material material = GenerateMaterial();
    osc::Material copy{material};
}

TEST_F(Renderer, MaterialCanBeMoveConstructed)
{
    osc::Material material = GenerateMaterial();
    osc::Material copy{std::move(material)};
}

TEST_F(Renderer, MaterialCanBeCopyAssigned)
{
    osc::Material m1 = GenerateMaterial();
    osc::Material m2 = GenerateMaterial();

    m1 = m2;
}

TEST_F(Renderer, MaterialCanBeMoveAssigned)
{
    osc::Material m1 = GenerateMaterial();
    osc::Material m2 = GenerateMaterial();

    m1 = std::move(m2);
}

TEST_F(Renderer, MaterialThatIsCopyConstructedEqualsSourceMaterial)
{
    osc::Material material = GenerateMaterial();
    osc::Material copy{material};

    ASSERT_EQ(material, copy);
}

TEST_F(Renderer, MaterialThatIsCopyAssignedEqualsSourceMaterial)
{
    osc::Material m1 = GenerateMaterial();
    osc::Material m2 = GenerateMaterial();

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST_F(Renderer, MaterialGetShaderReturnsSuppliedShader)
{
    osc::Shader shader{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::Material material{shader};

    ASSERT_EQ(material.getShader(), shader);
}

TEST_F(Renderer, MaterialGetColorOnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getColor("someKey"));
}

TEST_F(Renderer, MaterialCanCallSetColorOnNewMaterial)
{
    osc::Material mat = GenerateMaterial();
    mat.setColor("someKey", osc::Color::red());
}

TEST_F(Renderer, MaterialCallingGetColorOnMaterialAfterSetColorReturnsTheColor)
{
    osc::Material mat = GenerateMaterial();
    mat.setColor("someKey", osc::Color::red());

    ASSERT_EQ(mat.getColor("someKey"), osc::Color::red());
}

TEST_F(Renderer, MaterialGetColorArrayReturnsEmptyOnNewMaterial)
{
    osc::Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getColorArray("someKey"));
}

TEST_F(Renderer, MaterialCanCallSetColorArrayOnNewMaterial)
{
    osc::Material mat = GenerateMaterial();
    osc::Color colors[] = {osc::Color::black(), osc::Color::blue()};

    mat.setColorArray("someKey", colors);
}

TEST_F(Renderer, MaterialCallingGetColorArrayOnMaterialAfterSettingThemReturnsTheSameColors)
{
    osc::Material mat = GenerateMaterial();
    osc::Color const colors[] = {osc::Color::red(), osc::Color::green(), osc::Color::blue()};
    osc::CStringView const key = "someKey";

    mat.setColorArray(key, colors);

    std::optional<nonstd::span<osc::Color const>> rv = mat.getColorArray(key);

    ASSERT_TRUE(rv);
    ASSERT_EQ(std::size(*rv), std::size(colors));
    ASSERT_TRUE(std::equal(std::begin(colors), std::end(colors), std::begin(*rv)));
}
TEST_F(Renderer, MaterialGetFloatOnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getFloat("someKey"));
}

TEST_F(Renderer, MaterialGetFloatArrayOnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getFloatArray("someKey"));
}

TEST_F(Renderer, MaterialGetVec2OnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec2("someKey"));
}

TEST_F(Renderer, MaterialGetVec3OnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec3("someKey"));
}

TEST_F(Renderer, MaterialGetVec3ArrayOnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec3Array("someKey"));
}

TEST_F(Renderer, MaterialGetVec4OnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec4("someKey"));
}

TEST_F(Renderer, MaterialGetMat3OnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getMat3("someKey"));
}

TEST_F(Renderer, MaterialGetMat4OnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getMat4("someKey"));
}

TEST_F(Renderer, MaterialGetIntOnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getInt("someKey"));
}

TEST_F(Renderer, MaterialGetBoolOnNewMaterialReturnsEmptyOptional)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getBool("someKey"));
}

TEST_F(Renderer, MaterialSetFloatOnMaterialCausesGetFloatToReturnTheProvidedValue)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    float value = GenerateFloat();

    mat.setFloat(key, value);

    ASSERT_EQ(*mat.getFloat(key), value);
}

TEST_F(Renderer, MaterialSetFloatArrayOnMaterialCausesGetFloatArrayToReturnTheProvidedValues)
{
    osc::Material mat = GenerateMaterial();
    std::string key = "someKey";
    std::array<float, 4> values = {GenerateFloat(), GenerateFloat(), GenerateFloat(), GenerateFloat()};

    ASSERT_FALSE(mat.getFloatArray(key));

    mat.setFloatArray(key, values);

    nonstd::span<float const> rv = *mat.getFloatArray(key);
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(Renderer, MaterialSetVec2OnMaterialCausesGetVec2ToReturnTheProvidedValue)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::vec2 value = GenerateVec2();

    mat.setVec2(key, value);

    ASSERT_EQ(*mat.getVec2(key), value);
}

TEST_F(Renderer, MaterialSetVec2AndThenSetVec3CausesGetVec2ToReturnEmpty)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::vec2 value = GenerateVec2();

    ASSERT_FALSE(mat.getVec2(key).has_value());

    mat.setVec2(key, value);

    ASSERT_TRUE(mat.getVec2(key).has_value());

    mat.setVec3(key, {});

    ASSERT_TRUE(mat.getVec3(key));
    ASSERT_FALSE(mat.getVec2(key));
}

TEST_F(Renderer, MaterialSetVec2CausesMaterialToCompareNotEqualToCopy)
{
    osc::Material mat = GenerateMaterial();
    osc::Material copy{mat};

    mat.setVec2("someKey", GenerateVec2());

    ASSERT_NE(mat, copy);
}

TEST_F(Renderer, MaterialSetVec3OnMaterialCausesGetVec3ToReturnTheProvidedValue)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::vec3 value = GenerateVec3();

    mat.setVec3(key, value);

    ASSERT_EQ(*mat.getVec3(key), value);
}

TEST_F(Renderer, MaterialSetVec3ArrayOnMaterialCausesGetVec3ArrayToReutrnTheProvidedValues)
{
    osc::Material mat = GenerateMaterial();
    std::string key = "someKey";
    std::array<glm::vec3, 4> values = {GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3()};

    ASSERT_FALSE(mat.getVec3Array(key));

    mat.setVec3Array(key, values);

    nonstd::span<glm::vec3 const> rv = *mat.getVec3Array(key);
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(Renderer, MaterialSetVec4OnMaterialCausesGetVec4ToReturnTheProvidedValue)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::vec4 value = GenerateVec4();

    mat.setVec4(key, value);

    ASSERT_EQ(*mat.getVec4(key), value);
}

TEST_F(Renderer, MaterialSetMat3OnMaterialCausesGetMat3ToReturnTheProvidedValue)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::mat3 value = GenerateMat3x3();

    mat.setMat3(key, value);

    ASSERT_EQ(*mat.getMat3(key), value);
}

TEST_F(Renderer, MaterialSetMat4OnMaterialCausesGetMat4ToReturnTheProvidedValue)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::mat4 value = GenerateMat4x4();

    mat.setMat4(key, value);

    ASSERT_EQ(*mat.getMat4(key), value);
}

TEST_F(Renderer, MaterialSetIntOnMaterialCausesGetIntToReturnTheProvidedValue)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    int value = GenerateInt();

    mat.setInt(key, value);

    ASSERT_EQ(*mat.getInt(key), value);
}

TEST_F(Renderer, MaterialSetBoolOnMaterialCausesGetBoolToReturnTheProvidedValue)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    bool value = GenerateBool();

    mat.setBool(key, value);

    ASSERT_EQ(*mat.getBool(key), value);
}

TEST_F(Renderer, MaterialSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    osc::Texture2D t = GenerateTexture();

    ASSERT_FALSE(mat.getTexture(key));

    mat.setTexture(key, t);

    ASSERT_TRUE(mat.getTexture(key));
}

TEST_F(Renderer, MaterialClearTextureOnMaterialCausesGetTextureToReturnNothing)
{
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    osc::Texture2D t = GenerateTexture();

    ASSERT_FALSE(mat.getTexture(key));

    mat.setTexture(key, t);

    ASSERT_TRUE(mat.getTexture(key));

    mat.clearTexture(key);

    ASSERT_FALSE(mat.getTexture(key));
}

TEST_F(Renderer, MaterialSetRenderTextureCausesGetRenderTextureToReturnTheTexture)
{
    osc::Material mat = GenerateMaterial();
    osc::RenderTexture renderTex = GenerateRenderTexture();
    std::string key = "someKey";

    ASSERT_FALSE(mat.getRenderTexture(key));

    mat.setRenderTexture(key, renderTex);

    ASSERT_EQ(*mat.getRenderTexture(key), renderTex);
}

TEST_F(Renderer, MaterialSetRenderTextureFollowedByClearRenderTextureClearsTheRenderTexture)
{
    osc::Material mat = GenerateMaterial();
    osc::RenderTexture renderTex = GenerateRenderTexture();
    std::string key = "someKey";

    ASSERT_FALSE(mat.getRenderTexture(key));

    mat.setRenderTexture(key, renderTex);

    ASSERT_EQ(*mat.getRenderTexture(key), renderTex);

    mat.clearRenderTexture(key);

    ASSERT_FALSE(mat.getRenderTexture(key));
}

TEST_F(Renderer, MaterialGetCubemapInitiallyReturnsNothing)
{
    osc::Material const mat = GenerateMaterial();

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());
}

TEST_F(Renderer, MaterialGetCubemapReturnsSomethingAfterSettingCubemap)
{
    osc::Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());

    osc::Cubemap const cubemap{1, osc::TextureFormat::RGBA32};

    mat.setCubemap("cubemap", cubemap);

    ASSERT_TRUE(mat.getCubemap("cubemap").has_value());
}

TEST_F(Renderer, MaterialGetCubemapReturnsTheCubemapThatWasLastSet)
{
    osc::Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());

    osc::Cubemap const firstCubemap{1, osc::TextureFormat::RGBA32};
    osc::Cubemap const secondCubemap{2, osc::TextureFormat::RGBA32};  // different

    mat.setCubemap("cubemap", firstCubemap);
    ASSERT_EQ(mat.getCubemap("cubemap"), firstCubemap);

    mat.setCubemap("cubemap", secondCubemap);
    ASSERT_EQ(mat.getCubemap("cubemap"), secondCubemap);
}

TEST_F(Renderer, MaterialClearCubemapClearsTheCubemap)
{
    osc::Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());

    osc::Cubemap const cubemap{1, osc::TextureFormat::RGBA32};

    mat.setCubemap("cubemap", cubemap);

    ASSERT_TRUE(mat.getCubemap("cubemap").has_value());

    mat.clearCubemap("cubemap");

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());
}

TEST_F(Renderer, MaterialGetTransparentIsInitiallyFalse)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getTransparent());
}

TEST_F(Renderer, MaterialSetTransparentBehavesAsExpected)
{
    osc::Material mat = GenerateMaterial();
    mat.setTransparent(true);
    ASSERT_TRUE(mat.getTransparent());
    mat.setTransparent(false);
    ASSERT_FALSE(mat.getTransparent());
    mat.setTransparent(true);
    ASSERT_TRUE(mat.getTransparent());
}

TEST_F(Renderer, MaterialGetDepthTestedIsInitiallyTrue)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_TRUE(mat.getDepthTested());
}

TEST_F(Renderer, MaterialSetDepthTestedBehavesAsExpected)
{
    osc::Material mat = GenerateMaterial();
    mat.setDepthTested(false);
    ASSERT_FALSE(mat.getDepthTested());
    mat.setDepthTested(true);
    ASSERT_TRUE(mat.getDepthTested());
    mat.setDepthTested(false);
    ASSERT_FALSE(mat.getDepthTested());
}

TEST_F(Renderer, MaterialGetDepthFunctionIsInitiallyDefault)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_EQ(mat.getDepthFunction(), osc::DepthFunction::Default);
}

TEST_F(Renderer, MaterialSetDepthFunctionBehavesAsExpected)
{
    osc::Material mat = GenerateMaterial();

    ASSERT_EQ(mat.getDepthFunction(), osc::DepthFunction::Default);

    static_assert(osc::DepthFunction::Default != osc::DepthFunction::LessOrEqual);

    mat.setDepthFunction(osc::DepthFunction::LessOrEqual);

    ASSERT_EQ(mat.getDepthFunction(), osc::DepthFunction::LessOrEqual);
}

TEST_F(Renderer, MaterialGetWireframeModeIsInitiallyFalse)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getWireframeMode());
}

TEST_F(Renderer, MaterialSetWireframeModeBehavesAsExpected)
{
    osc::Material mat = GenerateMaterial();
    mat.setWireframeMode(false);
    ASSERT_FALSE(mat.getWireframeMode());
    mat.setWireframeMode(true);
    ASSERT_TRUE(mat.getWireframeMode());
    mat.setWireframeMode(false);
    ASSERT_FALSE(mat.getWireframeMode());
}

TEST_F(Renderer, MaterialSetWireframeModeCausesMaterialCopiesToReturnNonEqual)
{
    osc::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getWireframeMode());
    osc::Material copy{mat};
    ASSERT_EQ(mat, copy);
    copy.setWireframeMode(true);
    ASSERT_NE(mat, copy);
}

TEST_F(Renderer, MaterialCanCompareEquals)
{
    osc::Material mat = GenerateMaterial();
    osc::Material copy{mat};

    ASSERT_EQ(mat, copy);
}

TEST_F(Renderer, MaterialCanCompareNotEquals)
{
    osc::Material m1 = GenerateMaterial();
    osc::Material m2 = GenerateMaterial();

    ASSERT_NE(m1, m2);
}

TEST_F(Renderer, MaterialCanPrintToStringStream)
{
    osc::Material m1 = GenerateMaterial();

    std::stringstream ss;
    ss << m1;
}

TEST_F(Renderer, MaterialOutputStringContainsUsefulInformation)
{
    osc::Material m1 = GenerateMaterial();
    std::stringstream ss;

    ss << m1;

    std::string str{ss.str()};

    ASSERT_TRUE(osc::ContainsSubstringCaseInsensitive(str, "Material"));

    // TODO: should print more useful info, such as number of props etc.
}

TEST_F(Renderer, MaterialSetFloatAndThenSetVec3CausesGetFloatToReturnEmpty)
{
    // compound test: when the caller sets a Vec3 then calling getInt with the same key should return empty
    osc::Material mat = GenerateMaterial();

    std::string key = "someKey";
    float floatValue = GenerateFloat();
    glm::vec3 vecValue = GenerateVec3();

    mat.setFloat(key, floatValue);

    ASSERT_TRUE(mat.getFloat(key));

    mat.setVec3(key, vecValue);

    ASSERT_TRUE(mat.getVec3(key));
    ASSERT_FALSE(mat.getFloat(key));
}

TEST_F(Renderer, MaterialPropertyBlockCanDefaultConstruct)
{
    osc::MaterialPropertyBlock mpb;
}

TEST_F(Renderer, MaterialPropertyBlockCanCopyConstruct)
{
    osc::MaterialPropertyBlock mpb;
    osc::MaterialPropertyBlock copy{mpb};
}

TEST_F(Renderer, MaterialPropertyBlockCanMoveConstruct)
{
    osc::MaterialPropertyBlock mpb;
    osc::MaterialPropertyBlock copy{std::move(mpb)};
}

TEST_F(Renderer, MaterialPropertyBlockCanCopyAssign)
{
    osc::MaterialPropertyBlock m1;
    osc::MaterialPropertyBlock m2;

    m1 = m2;
}

TEST_F(Renderer, MaterialPropertyBlockCanMoveAssign)
{
    osc::MaterialPropertyBlock m1;
    osc::MaterialPropertyBlock m2;

    m1 = std::move(m2);
}

TEST_F(Renderer, MaterialPropertyBlock)
{
    osc::MaterialPropertyBlock mpb;

    ASSERT_TRUE(mpb.isEmpty());
}

TEST_F(Renderer, MaterialPropertyBlockCanClearDefaultConstructed)
{
    osc::MaterialPropertyBlock mpb;
    mpb.clear();

    ASSERT_TRUE(mpb.isEmpty());
}

TEST_F(Renderer, MaterialPropertyBlockClearClearsProperties)
{
    osc::MaterialPropertyBlock mpb;

    mpb.setFloat("someKey", GenerateFloat());

    ASSERT_FALSE(mpb.isEmpty());

    mpb.clear();

    ASSERT_TRUE(mpb.isEmpty());
}

TEST_F(Renderer, MaterialPropertyBlockGetColorOnNewMPBlReturnsEmptyOptional)
{
    osc::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getColor("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockCanCallSetColorOnNewMaterial)
{
    osc::MaterialPropertyBlock mpb;
    mpb.setColor("someKey", osc::Color::red());
}

TEST_F(Renderer, MaterialPropertyBlockCallingGetColorOnMPBAfterSetColorReturnsTheColor)
{
    osc::MaterialPropertyBlock mpb;
    mpb.setColor("someKey", osc::Color::red());

    ASSERT_EQ(mpb.getColor("someKey"), osc::Color::red());
}

TEST_F(Renderer, MaterialPropertyBlockGetFloatReturnsEmptyOnDefaultConstructedInstance)
{
    osc::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getFloat("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetVec3ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getVec3("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetVec4ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getVec4("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetMat3ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getMat3("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetMat4ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getMat4("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetIntReturnsEmptyOnDefaultConstructedInstance)
{
    osc::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getInt("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetBoolReturnsEmptyOnDefaultConstructedInstance)
{
    osc::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getBool("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockSetFloatCausesGetterToReturnSetValue)
{
    osc::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    float value = GenerateFloat();

    ASSERT_FALSE(mpb.getFloat(key));

    mpb.setFloat(key, value);
    ASSERT_TRUE(mpb.getFloat(key));
    ASSERT_EQ(mpb.getFloat(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetVec3CausesGetterToReturnSetValue)
{
    osc::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::vec3 value = GenerateVec3();

    ASSERT_FALSE(mpb.getVec3(key));

    mpb.setVec3(key, value);
    ASSERT_TRUE(mpb.getVec3(key));
    ASSERT_EQ(mpb.getVec3(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetVec4CausesGetterToReturnSetValue)
{
    osc::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::vec4 value = GenerateVec4();

    ASSERT_FALSE(mpb.getVec4(key));

    mpb.setVec4(key, value);
    ASSERT_TRUE(mpb.getVec4(key));
    ASSERT_EQ(mpb.getVec4(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetMat3CausesGetterToReturnSetValue)
{
    osc::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::mat3 value = GenerateMat3x3();

    ASSERT_FALSE(mpb.getVec4(key));

    mpb.setMat3(key, value);
    ASSERT_TRUE(mpb.getMat3(key));
    ASSERT_EQ(mpb.getMat3(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetIntCausesGetterToReturnSetValue)
{
    osc::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    int value = GenerateInt();

    ASSERT_FALSE(mpb.getInt(key));

    mpb.setInt(key, value);
    ASSERT_TRUE(mpb.getInt(key));
    ASSERT_EQ(mpb.getInt(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetBoolCausesGetterToReturnSetValue)
{
    osc::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    bool value = GenerateBool();

    ASSERT_FALSE(mpb.getBool(key));

    mpb.setBool(key, value);
    ASSERT_TRUE(mpb.getBool(key));
    ASSERT_EQ(mpb.getBool(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    osc::MaterialPropertyBlock mpb;

    std::string key = "someKey";
    osc::Texture2D t = GenerateTexture();

    ASSERT_FALSE(mpb.getTexture(key));

    mpb.setTexture(key, t);

    ASSERT_TRUE(mpb.getTexture(key));
}

TEST_F(Renderer, MaterialPropertyBlockCanCompareEquals)
{
    osc::MaterialPropertyBlock m1;
    osc::MaterialPropertyBlock m2;

    ASSERT_TRUE(m1 == m2);
}

TEST_F(Renderer, MaterialPropertyBlockCopyConstructionComparesEqual)
{
    osc::MaterialPropertyBlock m;
    osc::MaterialPropertyBlock copy{m};

    ASSERT_EQ(m, copy);
}

TEST_F(Renderer, MaterialPropertyBlockCopyAssignmentComparesEqual)
{
    osc::MaterialPropertyBlock m1;
    osc::MaterialPropertyBlock m2;

    m1.setFloat("someKey", GenerateFloat());

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST_F(Renderer, MaterialPropertyBlockDifferentMaterialBlocksCompareNotEqual)
{
    osc::MaterialPropertyBlock m1;
    osc::MaterialPropertyBlock m2;

    m1.setFloat("someKey", GenerateFloat());

    ASSERT_NE(m1, m2);
}

TEST_F(Renderer, MaterialPropertyBlockCanPrintToOutputStream)
{
    osc::MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;  // just ensure this compiles and runs
}

TEST_F(Renderer, MaterialPropertyBlockPrintingToOutputStreamMentionsMaterialPropertyBlock)
{
    osc::MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;

    ASSERT_TRUE(osc::ContainsSubstring(ss.str(), "MaterialPropertyBlock"));
}

TEST_F(Renderer, TextureCanConstructFromRGBAPixels)
{
    std::vector<osc::Rgba32> pixels(4);
    osc::Texture2D t{{2, 2}, pixels, osc::ColorSpace::sRGB};
}

TEST_F(Renderer, TextureRGBAThrowsIfDimensionsDontMatchNumberOfPixels)
{
    std::vector<osc::Rgba32> pixels(4);
    ASSERT_ANY_THROW({ osc::Texture2D t({1, 2}, pixels, osc::ColorSpace::sRGB); });
}

TEST_F(Renderer, TextureWithRuntimeNumberOfChannelsWorksForSingleChannelData)
{
    std::vector<std::uint8_t> singleChannelPixels(16);
    osc::Texture2D t{{4, 4}, osc::TextureFormat::R8, singleChannelPixels, osc::ColorSpace::sRGB};

    ASSERT_EQ(t.getDimensions(), glm::ivec2(4,4));
    ASSERT_EQ(t.getAspectRatio(), 1.0f);
}

TEST_F(Renderer, TextureWithRuntimeNumberOfChannelsWorksForRGBData)
{
    std::vector<std::uint8_t> rgbPixels(12);
    osc::Texture2D t{{2, 2}, osc::TextureFormat::RGB24, rgbPixels, osc::ColorSpace::sRGB};

    ASSERT_EQ(t.getDimensions(), glm::ivec2(2,2));
    ASSERT_EQ(t.getAspectRatio(), 1.0f);
}

TEST_F(Renderer, TextureWithRuntimeNumberOfChannelsWorksForRGBAData)
{
    std::vector<std::uint8_t> rgbaPixels(16);
    osc::Texture2D t{{2, 2}, osc::TextureFormat::RGBA32, rgbaPixels, osc::ColorSpace::sRGB};

    ASSERT_EQ(t.getDimensions(), glm::ivec2(2,2));
    ASSERT_EQ(t.getAspectRatio(), 1.0f);
}

TEST_F(Renderer, TextureCanCopyConstruct)
{
    osc::Texture2D t = GenerateTexture();
    osc::Texture2D copy{t};
}

TEST_F(Renderer, TextureCanMoveConstruct)
{
    osc::Texture2D t = GenerateTexture();
    osc::Texture2D copy{std::move(t)};
}

TEST_F(Renderer, TextureCanCopyAssign)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2 = GenerateTexture();

    t1 = t2;
}

TEST_F(Renderer, TextureCanMoveAssign)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2 = GenerateTexture();

    t1 = std::move(t2);
}

TEST_F(Renderer, TextureGetWidthReturnsSuppliedWidth)
{
    int width = 2;
    int height = 6;
    std::vector<osc::Rgba32> pixels(width*height);

    osc::Texture2D t{{width, height}, pixels, osc::ColorSpace::sRGB};

    ASSERT_EQ(t.getDimensions().x, width);
}

TEST_F(Renderer, TextureGetHeightReturnsSuppliedHeight)
{
    int width = 2;
    int height = 6;
    std::vector<osc::Rgba32> pixels(width*height);

    osc::Texture2D t{{width, height}, pixels, osc::ColorSpace::sRGB};

    ASSERT_EQ(t.getDimensions().y, height);
}

TEST_F(Renderer, TextureGetAspectRatioReturnsExpectedRatio)
{
    int width = 16;
    int height = 37;
    std::vector<osc::Rgba32> pixels(width*height);

    osc::Texture2D t{{width, height}, pixels, osc::ColorSpace::sRGB};

    float expected = static_cast<float>(width) / static_cast<float>(height);

    ASSERT_FLOAT_EQ(t.getAspectRatio(), expected);
}

TEST_F(Renderer, TextureGetColorSpaceReturnsProvidedColorSpaceIfSRGB)
{
    osc::Rgba32 pixels[] = {{}};
    osc::Texture2D t{{1, 1}, pixels, osc::ColorSpace::sRGB};

    ASSERT_EQ(t.getColorSpace(), osc::ColorSpace::sRGB);
}

TEST_F(Renderer, TextureGetColorSpaceReturnsProvidedColorSpaceIfLinear)
{
    osc::Rgba32 pixels[] = {{}};
    osc::Texture2D t{{1, 1}, pixels, osc::ColorSpace::Linear};

    ASSERT_EQ(t.getColorSpace(), osc::ColorSpace::Linear);
}

TEST_F(Renderer, TextureGetWrapModeReturnsRepeatedByDefault)
{
    osc::Texture2D t = GenerateTexture();

    ASSERT_EQ(t.getWrapMode(), osc::TextureWrapMode::Repeat);
}

TEST_F(Renderer, TextureSetWrapModeMakesSubsequentGetWrapModeReturnNewWrapMode)
{
    osc::Texture2D t = GenerateTexture();

    osc::TextureWrapMode wm = osc::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapMode(), wm);

    t.setWrapMode(wm);

    ASSERT_EQ(t.getWrapMode(), wm);
}

TEST_F(Renderer, TextureSetWrapModeCausesGetWrapModeUToAlsoReturnNewWrapMode)
{
    osc::Texture2D t = GenerateTexture();

    osc::TextureWrapMode wm = osc::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapMode(), wm);
    ASSERT_NE(t.getWrapModeU(), wm);

    t.setWrapMode(wm);

    ASSERT_EQ(t.getWrapModeU(), wm);
}

TEST_F(Renderer, TextureSetWrapModeUCausesGetWrapModeUToReturnValue)
{
    osc::Texture2D t = GenerateTexture();

    osc::TextureWrapMode wm = osc::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeU(), wm);

    t.setWrapModeU(wm);

    ASSERT_EQ(t.getWrapModeU(), wm);
}

TEST_F(Renderer, TextureSetWrapModeVCausesGetWrapModeVToReturnValue)
{
    osc::Texture2D t = GenerateTexture();

    osc::TextureWrapMode wm = osc::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeV(), wm);

    t.setWrapModeV(wm);

    ASSERT_EQ(t.getWrapModeV(), wm);
}

TEST_F(Renderer, TextureSetWrapModeWCausesGetWrapModeWToReturnValue)
{
    osc::Texture2D t = GenerateTexture();

    osc::TextureWrapMode wm = osc::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeW(), wm);

    t.setWrapModeW(wm);

    ASSERT_EQ(t.getWrapModeW(), wm);
}

TEST_F(Renderer, TextureSetFilterModeCausesGetFilterModeToReturnValue)
{
    osc::Texture2D t = GenerateTexture();

    osc::TextureFilterMode tfm = osc::TextureFilterMode::Linear;

    ASSERT_NE(t.getFilterMode(), tfm);

    t.setFilterMode(tfm);

    ASSERT_EQ(t.getFilterMode(), tfm);
}

TEST_F(Renderer, TextureSetFilterModeMipmapReturnsMipmapOnGetFilterMode)
{
    osc::Texture2D t = GenerateTexture();

    osc::TextureFilterMode tfm = osc::TextureFilterMode::Mipmap;

    ASSERT_NE(t.getFilterMode(), tfm);

    t.setFilterMode(tfm);

    ASSERT_EQ(t.getFilterMode(), tfm);
}

TEST_F(Renderer, TextureCanBeComparedForEquality)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2 = GenerateTexture();

    (void)(t1 == t2);  // just ensure it compiles + runs
}

TEST_F(Renderer, TextureCopyConstructingComparesEqual)
{
    osc::Texture2D t = GenerateTexture();
    osc::Texture2D tcopy{t};

    ASSERT_EQ(t, tcopy);
}

TEST_F(Renderer, TextureCopyAssignmentMakesEqualityReturnTrue)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2 = GenerateTexture();

    t1 = t2;

    ASSERT_EQ(t1, t2);
}

TEST_F(Renderer, TextureCanBeComparedForNotEquals)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2 = GenerateTexture();

    (void)(t1 != t2);  // just ensure this expression compiles
}

TEST_F(Renderer, TextureChangingWrapModeMakesCopyUnequal)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2{t1};
    osc::TextureWrapMode wm = osc::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapMode(), wm);

    t2.setWrapMode(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingWrapModeUMakesCopyUnequal)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2{t1};
    osc::TextureWrapMode wm = osc::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeU(), wm);

    t2.setWrapModeU(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingWrapModeVMakesCopyUnequal)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2{t1};
    osc::TextureWrapMode wm = osc::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeV(), wm);

    t2.setWrapModeV(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingWrapModeWMakesCopyUnequal)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2{t1};
    osc::TextureWrapMode wm = osc::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeW(), wm);

    t2.setWrapModeW(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingFilterModeMakesCopyUnequal)
{
    osc::Texture2D t1 = GenerateTexture();
    osc::Texture2D t2{t1};
    osc::TextureFilterMode fm = osc::TextureFilterMode::Linear;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getFilterMode(), fm);

    t2.setFilterMode(fm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureCanBeWrittenToOutputStream)
{
    osc::Texture2D t = GenerateTexture();

    std::stringstream ss;
    ss << t;

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(Renderer, MeshTopologyAllCanBeWrittenToStream)
{
    for (int i = 0; i < static_cast<int>(osc::MeshTopology::TOTAL); ++i)
    {
        osc::MeshTopology mt = static_cast<osc::MeshTopology>(i);

        std::stringstream ss;

        ss << mt;

        ASSERT_FALSE(ss.str().empty());
    }
}

TEST_F(Renderer, LoadTexture2DFromImageResourceCanLoadImageFile)
{
    osc::Texture2D const t = osc::LoadTexture2DFromImage(
        osc::App::resource("textures/awesomeface.png"),
        osc::ColorSpace::sRGB
    );
    ASSERT_EQ(t.getDimensions(), glm::ivec2(512, 512));
}

TEST_F(Renderer, LoadTexture2DFromImageResourceThrowsIfResourceNotFound)
{
    ASSERT_ANY_THROW(
    {
        osc::LoadTexture2DFromImage(
            osc::App::resource("textures/doesnt_exist.png"),
            osc::ColorSpace::sRGB
        );
    });
}

TEST_F(Renderer, MeshCanBeDefaultConstructed)
{
    osc::Mesh mesh;
}

TEST_F(Renderer, MeshCanBeCopyConstructed)
{
    osc::Mesh m;
    osc::Mesh copy{m};
}

TEST_F(Renderer, MeshCanBeMoveConstructed)
{
    osc::Mesh m1;
    osc::Mesh m2{std::move(m1)};
}

TEST_F(Renderer, MeshCanBeCopyAssigned)
{
    osc::Mesh m1;
    osc::Mesh m2;

    m1 = m2;
}

TEST_F(Renderer, MeshCanMeMoveAssigned)
{
    osc::Mesh m1;
    osc::Mesh m2;

    m1 = std::move(m2);
}

TEST_F(Renderer, MeshCanGetTopology)
{
    osc::Mesh m;

    m.getTopology();
}

TEST_F(Renderer, MeshGetTopologyDefaultsToTriangles)
{
    osc::Mesh m;

    ASSERT_EQ(m.getTopology(), osc::MeshTopology::Triangles);
}

TEST_F(Renderer, MeshSetTopologyCausesGetTopologyToUseSetValue)
{
    osc::Mesh m;
    osc::MeshTopology topography = osc::MeshTopology::Lines;

    ASSERT_NE(m.getTopology(), osc::MeshTopology::Lines);

    m.setTopology(topography);

    ASSERT_EQ(m.getTopology(), topography);
}

TEST_F(Renderer, MeshSetTopologyCausesCopiedMeshTobeNotEqualToInitialMesh)
{
    osc::Mesh m;
    osc::Mesh copy{m};
    osc::MeshTopology topology = osc::MeshTopology::Lines;

    ASSERT_EQ(m, copy);
    ASSERT_NE(copy.getTopology(), topology);

    copy.setTopology(topology);

    ASSERT_NE(m, copy);
}

TEST_F(Renderer, MeshGetVertsReturnsEmptyVertsOnDefaultConstruction)
{
    osc::Mesh m;
    ASSERT_TRUE(m.getVerts().empty());
}

TEST_F(Renderer, MeshSetVertsMakesGetCallReturnVerts)
{
    osc::Mesh m;
    std::vector<glm::vec3> verts = GenerateTriangleVerts();

    ASSERT_FALSE(SpansEqual(m.getVerts(), nonstd::span<glm::vec3 const>(verts)));
}

TEST_F(Renderer, MeshSetVertsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    osc::Mesh m;
    osc::Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.setVerts(GenerateTriangleVerts());

    ASSERT_NE(m, copy);
}

TEST_F(Renderer, MeshTransformVertsMakesGetCallReturnVerts)
{
    osc::Mesh m;

    // generate "original" verts
    std::vector<glm::vec3> originalVerts = GenerateTriangleVerts();

    // create "transformed" version of the verts
    std::vector<glm::vec3> newVerts;
    newVerts.reserve(originalVerts.size());
    for (glm::vec3 const& v : originalVerts)
    {
        newVerts.push_back(v + 1.0f);
    }

    // sanity check that `setVerts` works as expected
    ASSERT_TRUE(m.getVerts().empty());
    m.setVerts(originalVerts);
    ASSERT_TRUE(SpansEqual(m.getVerts(), nonstd::span<glm::vec3 const>(originalVerts)));

    // the verts passed to `transformVerts` should match those returned by getVerts
    m.transformVerts([&originalVerts](nonstd::span<glm::vec3 const> verts)
    {
        ASSERT_TRUE(SpansEqual(nonstd::span<glm::vec3 const>(originalVerts), verts));
    });

    // applying the transformation should return the transformed verts
    m.transformVerts([&newVerts](nonstd::span<glm::vec3> verts)
    {
        ASSERT_EQ(newVerts.size(), verts.size());
        for (size_t i = 0; i < verts.size(); ++i)
        {
            verts[i] = newVerts[i];
        }
    });
    ASSERT_TRUE(SpansEqual(m.getVerts(), nonstd::span<glm::vec3 const>(newVerts)));
}

TEST_F(Renderer, MeshTransformVertsCausesTransformedMeshToNotBeEqualToInitialMesh)
{
    osc::Mesh m;
    osc::Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.transformVerts([](nonstd::span<glm::vec3>) {});  // noop transform also triggers this (meshes aren't value-comparable)

    ASSERT_NE(m, copy);
}

TEST_F(Renderer, MeshGetNormalsReturnsEmptyOnDefaultConstruction)
{
    osc::Mesh m;
    ASSERT_TRUE(m.getNormals().empty());
}

TEST_F(Renderer, MeshSetNormalsMakesGetCallReturnSuppliedData)
{
    osc::Mesh m;
    std::vector<glm::vec3> normals = {GenerateVec3(), GenerateVec3(), GenerateVec3()};

    ASSERT_TRUE(m.getNormals().empty());

    m.setNormals(normals);

    ASSERT_TRUE(SpansEqual(m.getNormals(), nonstd::span<glm::vec3 const>(normals)));
}

TEST_F(Renderer, MeshSetNormalsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    osc::Mesh m;
    osc::Mesh copy{m};
    std::vector<glm::vec3> normals = {GenerateVec3(), GenerateVec3(), GenerateVec3()};

    ASSERT_EQ(m, copy);

    copy.setNormals(normals);

    ASSERT_NE(m, copy);
}

TEST_F(Renderer, MeshGetTexCoordsReturnsEmptyOnDefaultConstruction)
{
    osc::Mesh m;
    ASSERT_TRUE(m.getTexCoords().empty());
}

TEST_F(Renderer, MeshSetTexCoordsCausesGetToReturnSuppliedData)
{
    osc::Mesh m;
    std::vector<glm::vec2> coords = {GenerateVec2(), GenerateVec2(), GenerateVec2()};

    ASSERT_TRUE(m.getTexCoords().empty());

    m.setTexCoords(coords);

    ASSERT_TRUE(SpansEqual(m.getTexCoords(), nonstd::span<glm::vec2 const>(coords)));
}

TEST_F(Renderer, MeshSetTexCoordsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    osc::Mesh m;
    osc::Mesh copy{m};
    std::vector<glm::vec2> coords = {GenerateVec2(), GenerateVec2(), GenerateVec2()};

    ASSERT_EQ(m, copy);

    copy.setTexCoords(coords);

    ASSERT_NE(m, copy);
}

TEST_F(Renderer, MeshGetColorsInitiallyReturnsEmptySpan)
{
    osc::Mesh m;
    ASSERT_TRUE(m.getColors().empty());
}

TEST_F(Renderer, MeshSetColorsFollowedByGetColorsReturnsColors)
{
    osc::Mesh m;
    std::array<osc::Rgba32, 3> colors{};

    m.setColors(colors);

    auto rv = m.getColors();
    ASSERT_EQ(rv.size(), colors.size());
}

TEST_F(Renderer, MeshGetTangentsInitiallyReturnsEmptySpan)
{
    osc::Mesh m;
    ASSERT_TRUE(m.getTangents().empty());
}

TEST_F(Renderer, MeshSetTangentsFollowedByGetTangentsReturnsTangents)
{
    osc::Mesh m;
    std::array<glm::vec4, 5> tangents{};

    m.setTangents(tangents);
    ASSERT_EQ(m.getTangents().size(), tangents.size());
}

TEST_F(Renderer, MeshGetNumIndicesReturnsZeroOnDefaultConstruction)
{
    osc::Mesh m;
    ASSERT_EQ(m.getIndices().size(), 0);
}

TEST_F(Renderer, MeshGetBoundsReturnsEmptyBoundsOnInitialization)
{
    osc::Mesh m;
    osc::AABB empty{};
    ASSERT_EQ(m.getBounds(), empty);
}

TEST_F(Renderer, MeshGetBoundsReturnsEmptyForMeshWithUnindexedVerts)
{
    glm::vec3 pyramid[] =
    {
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
        { 0.0f,  0.0f, 1.0f},  // tip
    };

    osc::Mesh m;
    m.setVerts(pyramid);
    osc::AABB empty{};
    ASSERT_EQ(m.getBounds(), empty);
}

TEST_F(Renderer, MeshGetBooundsReturnsNonemptyForIndexedVerts)
{
    glm::vec3 pyramid[] =
    {
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    };
    std::uint16_t pyramidIndices[] = {0, 1, 2};

    osc::Mesh m;
    m.setVerts(pyramid);
    m.setIndices(pyramidIndices);
    osc::AABB expected = osc::AABBFromVerts(pyramid);
    ASSERT_EQ(m.getBounds(), expected);
}

TEST_F(Renderer, MeshGetMidpointReturnsZeroVecOnInitialization)
{
    osc::Mesh m;
    glm::vec3 expected = {};
    ASSERT_EQ(m.getMidpoint(), expected);
}

TEST_F(Renderer, MeshGetMidpointReturnsExpectedMidpoint)
{
    glm::vec3 pyramid[] =
    {
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    };
    std::uint16_t pyramidIndices[] = {0, 1, 2};

    osc::Mesh m;
    m.setVerts(pyramid);
    m.setIndices(pyramidIndices);
    glm::vec3 expected = osc::Midpoint(osc::AABBFromVerts(pyramid));
    ASSERT_EQ(m.getMidpoint(), expected);
}

TEST_F(Renderer, MeshGetBVHReturnsEmptyBVHOnInitialization)
{
    osc::Mesh m;
    osc::BVH const& bvh = m.getBVH();
    ASSERT_TRUE(bvh.nodes.empty());
}

TEST_F(Renderer, MeshGetBVHReturnsExpectedRootNode)
{
    glm::vec3 pyramid[] =
    {
        {-1.0f, -1.0f, 0.0f},  // base: bottom-left
        { 1.0f, -1.0f, 0.0f},  // base: bottom-right
        { 0.0f,  1.0f, 0.0f},  // base: top-middle
    };
    std::uint16_t pyramidIndices[] = {0, 1, 2};

    osc::Mesh m;
    m.setVerts(pyramid);
    m.setIndices(pyramidIndices);

    osc::AABB const expectedRoot = osc::AABBFromVerts(pyramid);

    osc::BVH const& bvh = m.getBVH();

    ASSERT_FALSE(bvh.nodes.empty());
    ASSERT_EQ(expectedRoot, bvh.nodes.front().getBounds());
}

TEST_F(Renderer, MeshCanBeComparedForEquality)
{
    osc::Mesh m1;
    osc::Mesh m2;

    (void)(m1 == m2);  // just ensure the expression compiles
}

TEST_F(Renderer, MeshCopiesAreEqual)
{
    osc::Mesh m;
    osc::Mesh copy{m};

    ASSERT_EQ(m, copy);
}

TEST_F(Renderer, MeshCanBeComparedForNotEquals)
{
    osc::Mesh m1;
    osc::Mesh m2;

    (void)(m1 != m2);  // just ensure the expression compiles
}

TEST_F(Renderer, MeshCanBeWrittenToOutputStreamForDebugging)
{
    osc::Mesh m;
    std::stringstream ss;

    ss << m;

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(Renderer, RenderTextureFormatCanBeIteratedOverAndStreamedToString)
{
    for (int i = 0; i < static_cast<int>(osc::RenderTextureFormat::TOTAL); ++i)
    {
        std::stringstream ss;
        ss << static_cast<osc::RenderTextureFormat>(i);  // shouldn't throw
    }
}

TEST_F(Renderer, DepthStencilFormatCanBeIteratedOverAndStreamedToString)
{
    for (int i = 0; i < static_cast<int>(osc::DepthStencilFormat::TOTAL); ++i)
    {
        std::stringstream ss;
        ss << static_cast<osc::DepthStencilFormat>(i);  // shouldn't throw
    }
}

TEST_F(Renderer, RenderTextureDescriptorCanBeConstructedFromWithAndHeight)
{
    osc::RenderTextureDescriptor d{{1, 1}};
}

TEST_F(Renderer, RenderTextureDescriptorCoercesNegativeWidthsToZero)
{
    osc::RenderTextureDescriptor d{{-1, 1}};

    ASSERT_EQ(d.getDimensions().x, 0);
}

TEST_F(Renderer, RenderTextureDescriptorCoercesNegativeHeightsToZero)
{
    osc::RenderTextureDescriptor d{{1, -1}};

    ASSERT_EQ(d.getDimensions().y, 0);
}

TEST_F(Renderer, RenderTextureDescriptorCanBeCopyConstructed)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{d1};
}

TEST_F(Renderer, RenderTextureDescriptorCanBeMoveConstructed)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{std::move(d1)};
}

TEST_F(Renderer, RenderTextureDescriptorCanBeCopyAssigned)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{{1, 1}};
    d1 = d2;
}

TEST_F(Renderer, RenderTextureDescriptorCanBeMoveAssigned)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{{1, 1}};
    d1 = std::move(d2);
}

TEST_F(Renderer, RenderTextureDescriptorGetWidthReturnsConstructedWith)
{
    int width = 1;
    osc::RenderTextureDescriptor d1{{width, 1}};
    ASSERT_EQ(d1.getDimensions().x, width);
}

TEST_F(Renderer, RenderTextureDescriptorSetWithFollowedByGetWithReturnsSetWidth)
{
    osc::RenderTextureDescriptor d1{{1, 1}};


    int newWidth = 31;
    glm::ivec2 d = d1.getDimensions();
    d.x = newWidth;

    d1.setDimensions(d);
    ASSERT_EQ(d1.getDimensions(), d);
}

TEST_F(Renderer, RenderTextureDescriptorSetWidthNegativeValueThrows)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    ASSERT_ANY_THROW({ d1.setDimensions({-1, 1}); });
}

TEST_F(Renderer, RenderTextureDescriptorGetHeightReturnsConstructedHeight)
{
    int height = 1;
    osc::RenderTextureDescriptor d1{{1, height}};
    ASSERT_EQ(d1.getDimensions().y, height);
}

TEST_F(Renderer, RenderTextureDescriptorSetHeightFollowedByGetHeightReturnsSetHeight)
{
    osc::RenderTextureDescriptor d1{{1, 1}};

    glm::ivec2 d = d1.getDimensions();
    d.y = 31;

    d1.setDimensions(d);

    ASSERT_EQ(d1.getDimensions(), d);
}

TEST_F(Renderer, RenderTextureDescriptorGetAntialiasingLevelInitiallyReturns1)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getAntialiasingLevel(), 1);
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelMakesGetAntialiasingLevelReturnValue)
{
    int32_t newAntialiasingLevel = 4;

    osc::RenderTextureDescriptor d1{{1, 1}};
    d1.setAntialiasingLevel(newAntialiasingLevel);
    ASSERT_EQ(d1.getAntialiasingLevel(), newAntialiasingLevel);
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelToZeroThrows)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    ASSERT_ANY_THROW({ d1.setAntialiasingLevel(0); });
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingNegativeThrows)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    ASSERT_ANY_THROW({ d1.setAntialiasingLevel(-1); });
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelToInvalidValueThrows)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    ASSERT_ANY_THROW({ d1.setAntialiasingLevel(3); });
}

TEST_F(Renderer, RenderTextureDescriptorGetColorFormatReturnsARGB32ByDefault)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getColorFormat(), osc::RenderTextureFormat::ARGB32);
}

TEST_F(Renderer, RenderTextureDescriptorSetColorFormatMakesGetColorFormatReturnTheFormat)
{
    osc::RenderTextureDescriptor d{{1, 1}};

    ASSERT_EQ(d.getColorFormat(), osc::RenderTextureFormat::ARGB32);

    d.setColorFormat(osc::RenderTextureFormat::RED);

    ASSERT_EQ(d.getColorFormat(), osc::RenderTextureFormat::RED);
}

TEST_F(Renderer, RenderTextureDescriptorGetDepthStencilFormatReturnsDefaultValue)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getDepthStencilFormat(), osc::DepthStencilFormat::D24_UNorm_S8_UInt);
}

TEST_F(Renderer, RenderTextureDescriptorStandardCtorGetReadWriteReturnsDefaultValue)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getReadWrite(), osc::RenderTextureReadWrite::Default);
}

TEST_F(Renderer, RenderTextureDescriptorSetReadWriteMakesGetReadWriteReturnNewValue)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    ASSERT_EQ(d1.getReadWrite(), osc::RenderTextureReadWrite::Default);

    d1.setReadWrite(osc::RenderTextureReadWrite::Linear);

    ASSERT_EQ(d1.getReadWrite(), osc::RenderTextureReadWrite::Linear);
}

TEST_F(Renderer, RenderTextureSetReadWriteChangesEquality)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{d1};

    ASSERT_EQ(d1, d2);

    d2.setReadWrite(osc::RenderTextureReadWrite::Linear);

    ASSERT_NE(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorComparesEqualOnCopyConstruct)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{d1};

    ASSERT_EQ(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorComparesEqualWithSameConstructionVals)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{{1, 1}};

    ASSERT_EQ(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorSetDimensionsWidthMakesItCompareNotEqual)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{{1, 1}};

    d2.setDimensions({2, 1});

    ASSERT_NE(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorSetDimensionsHeightMakesItCompareNotEqual)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{{1, 1}};

    d2.setDimensions({1, 2});

    ASSERT_NE(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelMakesItCompareNotEqual)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{{1, 1}};

    d2.setAntialiasingLevel(2);

    ASSERT_NE(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelToSameValueComparesEqual)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTextureDescriptor d2{{1, 1}};

    d2.setAntialiasingLevel(d2.getAntialiasingLevel());

    ASSERT_EQ(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorCanBeStreamedToAString)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    std::stringstream ss;
    ss << d1;

    std::string str{ss.str()};
    ASSERT_TRUE(osc::ContainsSubstringCaseInsensitive(str, "RenderTextureDescriptor"));
}

TEST_F(Renderer, RenderTextureDefaultConstructorCreates1x1RgbaRenderTexture)
{
    osc::RenderTexture const tex;
    ASSERT_EQ(tex.getDimensions(), glm::ivec2(1, 1));
    ASSERT_EQ(tex.getDepthStencilFormat(), osc::DepthStencilFormat::D24_UNorm_S8_UInt);
    ASSERT_EQ(tex.getColorFormat(), osc::RenderTextureFormat::ARGB32);
    ASSERT_EQ(tex.getAntialiasingLevel(), 1);
}

TEST_F(Renderer, RenderTextureCanBeConstructedFromDimensions)
{
    glm::ivec2 const dims = {12, 12};
    osc::RenderTexture tex{dims};
    ASSERT_EQ(tex.getDimensions(), dims);
}

TEST_F(Renderer, RenderTextureCanBeConstructedFromADescriptor)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTexture d{d1};
}

TEST_F(Renderer, RenderTextureDefaultCtorAssignsDefaultReadWrite)
{
    osc::RenderTexture t;

    ASSERT_EQ(t.getReadWrite(), osc::RenderTextureReadWrite::Default);
}

TEST_F(Renderer, RenderTextureFromDescriptorHasExpectedValues)
{
    int width = 5;
    int height = 8;
    int32_t aaLevel = 4;
    osc::RenderTextureFormat format = osc::RenderTextureFormat::RED;
    osc::RenderTextureReadWrite rw = osc::RenderTextureReadWrite::Linear;

    osc::RenderTextureDescriptor desc{{width, height}};
    desc.setAntialiasingLevel(aaLevel);
    desc.setColorFormat(format);
    desc.setReadWrite(rw);

    osc::RenderTexture tex{desc};

    ASSERT_EQ(tex.getDimensions(), glm::ivec2(width, height));
    ASSERT_EQ(tex.getAntialiasingLevel(), aaLevel);
    ASSERT_EQ(tex.getColorFormat(), format);
    ASSERT_EQ(tex.getReadWrite(), rw);
}

TEST_F(Renderer, RenderTextureSetColorFormatCausesGetColorFormatToReturnValue)
{
    osc::RenderTextureDescriptor d1{{1, 1}};
    osc::RenderTexture d{d1};

    ASSERT_EQ(d.getColorFormat(), osc::RenderTextureFormat::ARGB32);

    d.setColorFormat(osc::RenderTextureFormat::RED);

    ASSERT_EQ(d.getColorFormat(), osc::RenderTextureFormat::RED);
}

TEST_F(Renderer, RenderTextureUpdColorBufferReturnsNonNullPtr)
{
    osc::RenderTexture rt{{1, 1}};

    ASSERT_NE(rt.updColorBuffer(), nullptr);
}

TEST_F(Renderer, RenderTextureUpdDepthBufferReturnsNonNullPtr)
{
    osc::RenderTexture rt{{1, 1}};

    ASSERT_NE(rt.updDepthBuffer(), nullptr);
}

TEST_F(Renderer, CameraProjectionCanBeStreamed)
{
    for (int i = 0; i < static_cast<int>(osc::CameraProjection::TOTAL); ++i)
    {
        std::stringstream ss;
        ss << static_cast<osc::CameraProjection>(i);

        ASSERT_FALSE(ss.str().empty());
    }
}

TEST_F(Renderer, CameraCanDefaultConstruct)
{
    osc::Camera camera;  // should compile + run
}

TEST_F(Renderer, CameraCanBeCopyConstructed)
{
    osc::Camera c;
    osc::Camera copy{c};
}

TEST_F(Renderer, CameraThatIsCopyConstructedComparesEqual)
{
    osc::Camera c;
    osc::Camera copy{c};

    ASSERT_EQ(c, copy);
}

TEST_F(Renderer, CameraCanBeMoveConstructed)
{
    osc::Camera c;
    osc::Camera copy{std::move(c)};
}

TEST_F(Renderer, CameraCanBeCopyAssigned)
{
    osc::Camera c1;
    osc::Camera c2;

    c2 = c1;
}

TEST_F(Renderer, CameraThatIsCopyAssignedComparesEqualToSource)
{
    osc::Camera c1;
    osc::Camera c2;

    c1 = c2;

    ASSERT_EQ(c1, c2);
}

TEST_F(Renderer, CameraCanBeMoveAssigned)
{
    osc::Camera c1;
    osc::Camera c2;

    c2 = std::move(c1);
}

TEST_F(Renderer, CameraUsesValueComparison)
{
    osc::Camera c1;
    osc::Camera c2;

    ASSERT_EQ(c1, c2);

    c1.setCameraFOV(1337.0f);

    ASSERT_NE(c1, c2);

    c2.setCameraFOV(1337.0f);

    ASSERT_EQ(c1, c2);
}

TEST_F(Renderer, CameraResetResetsToDefaultValues)
{
    osc::Camera const defaultCamera;
    osc::Camera camera = defaultCamera;
    camera.setDirection({1.0f, 0.0f, 0.0f});
    ASSERT_NE(camera, defaultCamera);
    camera.reset();
    ASSERT_EQ(camera, defaultCamera);
}

TEST_F(Renderer, CameraCanGetBackgroundColor)
{
    osc::Camera camera;

    ASSERT_EQ(camera.getBackgroundColor(), osc::Color::clear());
}

TEST_F(Renderer, CameraCanSetBackgroundColor)
{
    osc::Camera camera;
    camera.setBackgroundColor(GenerateColor());
}

TEST_F(Renderer, CameraSetBackgroundColorMakesGetBackgroundColorReturnTheColor)
{
    osc::Camera camera;
    osc::Color const color = GenerateColor();

    camera.setBackgroundColor(color);

    ASSERT_EQ(camera.getBackgroundColor(), color);
}

TEST_F(Renderer, CameraSetBackgroundColorMakesCameraCompareNonEqualWithCopySource)
{
    osc::Camera camera;
    osc::Camera copy{camera};

    ASSERT_EQ(camera, copy);

    copy.setBackgroundColor(GenerateColor());

    ASSERT_NE(camera, copy);
}

TEST_F(Renderer, CameraGetCameraProjectionReturnsProject)
{
    osc::Camera camera;
    ASSERT_EQ(camera.getCameraProjection(), osc::CameraProjection::Perspective);
}

TEST_F(Renderer, CameraCanSetCameraProjection)
{
    osc::Camera camera;
    camera.setCameraProjection(osc::CameraProjection::Orthographic);
}

TEST_F(Renderer, CameraSetCameraProjectionMakesGetCameraProjectionReturnSetProjection)
{
    osc::Camera camera;
    osc::CameraProjection proj = osc::CameraProjection::Orthographic;

    ASSERT_NE(camera.getCameraProjection(), proj);

    camera.setCameraProjection(proj);

    ASSERT_EQ(camera.getCameraProjection(), proj);
}

TEST_F(Renderer, CameraSetCameraProjectionMakesCameraCompareNotEqual)
{
    osc::Camera camera;
    osc::Camera copy{camera};
    osc::CameraProjection proj = osc::CameraProjection::Orthographic;

    ASSERT_NE(copy.getCameraProjection(), proj);

    copy.setCameraProjection(proj);

    ASSERT_NE(camera, copy);
}

TEST_F(Renderer, CameraGetPositionReturnsOriginOnDefaultConstruction)
{
    osc::Camera camera;
    ASSERT_EQ(camera.getPosition(), glm::vec3(0.0f, 0.0f, 0.0f));
}

TEST_F(Renderer, CameraSetDirectionToStandardDirectionCausesGetDirectionToReturnTheDirection)
{
    // this test kind of sucks, because it's assuming that the direction isn't touched if it's
    // a default one - that isn't strictly true because it is identity transformed
    //
    // the main reason this test exists is just to sanity-check parts of the direction API

    osc::Camera camera;

    glm::vec3 defaultDirection = {0.0f, 0.0f, -1.0f};

    ASSERT_EQ(camera.getDirection(), defaultDirection);

    glm::vec3 differentDirection = glm::normalize(glm::vec3{1.0f, 2.0f, -0.5f});
    camera.setDirection(differentDirection);

    // not guaranteed: the camera stores *rotation*, not *direction*
    (void)(camera.getDirection() == differentDirection);  // just ensure it compiles

    camera.setDirection(defaultDirection);

    ASSERT_EQ(camera.getDirection(), defaultDirection);
}

TEST_F(Renderer, CameraSetDirectionToDifferentDirectionGivesAccurateEnoughResults)
{
    // this kind of test sucks, because it's effectively saying "is the result good enough"
    //
    // the reason why the camera can't be *precise* about storing directions is because it
    // only guarantees storing the position + rotation accurately - the Z direction vector
    // is computed *from*  the rotation and may change a little bit between set/get

    osc::Camera camera;

    glm::vec3 newDirection = glm::normalize(glm::vec3{1.0f, 1.0f, 1.0f});

    camera.setDirection(newDirection);

    glm::vec3 returnedDirection = camera.getDirection();

    ASSERT_GT(glm::dot(newDirection, returnedDirection), 0.999f);
}

TEST_F(Renderer, CameraGetViewMatrixReturnsViewMatrixBasedOnPositonDirectionAndUp)
{
    osc::Camera camera;
    camera.setCameraProjection(osc::CameraProjection::Orthographic);
    camera.setPosition({0.0f, 0.0f, 0.0f});

    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 expectedMatrix{1.0f};

    ASSERT_EQ(viewMatrix, expectedMatrix);
}

TEST_F(Renderer, CameraSetViewMatrixOverrideSetsANewViewMatrixThatCanBeRetrievedWithGetViewMatrix)
{
    osc::Camera camera;

    // these shouldn't matter - they're overridden
    camera.setCameraProjection(osc::CameraProjection::Orthographic);
    camera.setPosition({7.0f, 5.0f, -3.0f});  

    glm::mat4 viewMatrix{1.0f};
    viewMatrix[0][1] = 9.0f;  // change some part of it

    camera.setViewMatrixOverride(viewMatrix);

    ASSERT_EQ(camera.getViewMatrix(), viewMatrix);
}

TEST_F(Renderer, CameraSetViewMatrixOverrideNulloptResetsTheViewMatrixToUsingStandardCameraPositionEtc)
{
    osc::Camera camera;
    glm::mat4 initialViewMatrix = camera.getViewMatrix();

    glm::mat4 viewMatrix{1.0f};
    viewMatrix[0][1] = 9.0f;  // change some part of it

    camera.setViewMatrixOverride(viewMatrix);
    ASSERT_NE(camera.getViewMatrix(), initialViewMatrix);
    ASSERT_EQ(camera.getViewMatrix(), viewMatrix);

    camera.setViewMatrixOverride(std::nullopt);

    ASSERT_EQ(camera.getViewMatrix(), initialViewMatrix);
}

TEST_F(Renderer, CameraGetProjectionMatrixReturnsProjectionMatrixBasedOnPositonDirectionAndUp)
{
    osc::Camera camera;
    camera.setCameraProjection(osc::CameraProjection::Orthographic);
    camera.setPosition({0.0f, 0.0f, 0.0f});

    glm::mat4 mtx = camera.getProjectionMatrix(1.0f);
    glm::mat4 expected{1.0f};

    // only compare the Y, Z, and W columns: the X column depends on the aspect ratio of the output
    // target
    ASSERT_EQ(mtx[1], expected[1]);
    ASSERT_EQ(mtx[2], expected[2]);
    ASSERT_EQ(mtx[3], expected[3]);
}

TEST_F(Renderer, CameraSetProjectionMatrixOverrideSetsANewProjectionMatrixThatCanBeRetrievedWithGetProjectionMatrix)
{
    osc::Camera camera;

    // these shouldn't matter - they're overridden
    camera.setCameraProjection(osc::CameraProjection::Orthographic);
    camera.setPosition({7.0f, 5.0f, -3.0f});  

    glm::mat4 ProjectionMatrix{1.0f};
    ProjectionMatrix[0][1] = 9.0f;  // change some part of it

    camera.setProjectionMatrixOverride(ProjectionMatrix);

    ASSERT_EQ(camera.getProjectionMatrix(1.0f), ProjectionMatrix);
}

TEST_F(Renderer, CameraSetProjectionMatrixNulloptResetsTheProjectionMatrixToUsingStandardCameraPositionEtc)
{
    osc::Camera camera;
    glm::mat4 initialProjectionMatrix = camera.getProjectionMatrix(1.0f);

    glm::mat4 ProjectionMatrix{1.0f};
    ProjectionMatrix[0][1] = 9.0f;  // change some part of it

    camera.setProjectionMatrixOverride(ProjectionMatrix);
    ASSERT_NE(camera.getProjectionMatrix(1.0f), initialProjectionMatrix);
    ASSERT_EQ(camera.getProjectionMatrix(1.0f), ProjectionMatrix);

    camera.setProjectionMatrixOverride(std::nullopt);

    ASSERT_EQ(camera.getProjectionMatrix(1.0f), initialProjectionMatrix);
}

TEST_F(Renderer, CameraGetViewProjectionMatrixReturnsViewMatrixMultipliedByProjectionMatrix)
{
    osc::Camera camera;

    glm::mat4 viewMatrix{1.0f};
    viewMatrix[0][3] = 2.5f;  // change some part of it

    glm::mat4 projectionMatrix{1.0f};
    projectionMatrix[0][1] = 9.0f;  // change some part of it

    glm::mat4 expected = projectionMatrix * viewMatrix;

    camera.setViewMatrixOverride(viewMatrix);
    camera.setProjectionMatrixOverride(projectionMatrix);

    ASSERT_EQ(camera.getViewProjectionMatrix(1.0f), expected);
}

TEST_F(Renderer, CameraGetInverseViewProjectionMatrixReturnsExpectedAnswerWhenUsingOverriddenMatrices)
{
    osc::Camera camera;

    glm::mat4 viewMatrix{1.0f};
    viewMatrix[0][3] = 2.5f;  // change some part of it

    glm::mat4 projectionMatrix{1.0f};
    projectionMatrix[0][1] = 9.0f;  // change some part of it

    glm::mat4 expected = glm::inverse(projectionMatrix * viewMatrix);

    camera.setViewMatrixOverride(viewMatrix);
    camera.setProjectionMatrixOverride(projectionMatrix);

    ASSERT_EQ(camera.getInverseViewProjectionMatrix(1.0f), expected);
}

TEST_F(Renderer, CameraGetClearFlagsReturnsSolidColorOnDefaultConstruction)
{
    static_assert(osc::CameraClearFlags::Default == osc::CameraClearFlags::SolidColor);

    osc::Camera camera;
    ASSERT_EQ(camera.getClearFlags(), osc::CameraClearFlags::SolidColor);
}

TEST_F(Renderer, CameraSetClearFlagsCausesGetClearFlagsToReturnNewValue)
{
    osc::Camera camera;

    ASSERT_EQ(camera.getClearFlags(), osc::CameraClearFlags::SolidColor);

    camera.setClearFlags(osc::CameraClearFlags::Nothing);

    ASSERT_EQ(camera.getClearFlags(), osc::CameraClearFlags::Nothing);
}

TEST_F(Renderer, CameraSetClearFlagsCausesCopyToReturnNonEqual)
{
    osc::Camera camera;
    osc::Camera copy{camera};

    ASSERT_EQ(camera, copy);
    ASSERT_EQ(camera.getClearFlags(), osc::CameraClearFlags::SolidColor);

    camera.setClearFlags(osc::CameraClearFlags::Nothing);

    ASSERT_NE(camera, copy);
}

// TODO MeshSetIndicesU16CausesGetNumIndicesToEqualSuppliedNumberOfIndices
// TODO Mesh::getIndices
// TODO Mesh::setIndices U16
// TODO Mesh::setIndices U32
// TODO Mesh::setIndices MeshIndicesView
// TODO Mesh ensure > 2^16 indices are allowed
// TODO Mesh::clear
// 
// TODO: RenderTexture (all)

// TODO: texture: ensure texture debug string contains useful information etc.

// TODO: Camera: orthographic size
// TODO: Camera: fov
// TODO: Camera: clipping planes
// TODO: Camera: texture
// TODO: Camera: pixel rect
// TODO: Camera: pixel dims
// TODO: Camera: scissor rect
// TODO: Camera: position
// TODO: Camera: direction
// TODO: Camera: up
// TODO: Camera: matrix
// TODO: Camera: render
// TODO: Camera: operator<<
// TODO: Camera: to_string
// TODO: Camera: hash
// TODO: Camera: ensure output strings are actually useful
