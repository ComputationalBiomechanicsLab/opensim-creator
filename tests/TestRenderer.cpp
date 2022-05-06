#include "src/Graphics/Renderer.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"

#include <gtest/gtest.h>

#include <array>
#include <sstream>
#include <string>
#include <unordered_set>

class ShaderTest : public ::testing::Test {
    osc::App m_App;
};

static constexpr char const g_VertexShaderSrc[] =
R"(
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

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    layout (location = 2) in vec3 aNormal;

    layout (location = 6) in mat4x3 aModelMat;
    layout (location = 10) in mat3 aNormalMat;
    layout (location = 13) in vec4 aRgba0;
    layout (location = 14) in float aRimIntensity;

    out vec4 GouraudBrightness;
    out vec4 Rgba0;
    out float RimIntensity;
    out vec2 TexCoord;

    const float ambientStrength = 0.7f;
    const float diffuseStrength = 0.3f;
    const float specularStrength = 0.1f;
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

static constexpr char const g_FragmentShaderSrc[] =
R"(
    #version 330 core

    uniform bool uIsTextured = false;
    uniform sampler2D uSampler0;

    in vec4 GouraudBrightness;
    in vec4 Rgba0;
    in float RimIntensity;
    in vec2 TexCoord;

    layout (location = 0) out vec4 Color0Out;
    layout (location = 1) out float Color1Out;

    void main()
    {
        vec4 color = uIsTextured ? texture(uSampler0, TexCoord) : Rgba0;
        color *= GouraudBrightness;

        Color0Out = color;
        Color1Out = RimIntensity;
    }
)";

// expected, based on the above shader code
static constexpr std::array<osc::CStringView, 7> g_ExpectedPropertyNames =
{
    "uProjMat",
    "uViewMat",
    "uLightDir",
    "uLightColor",
    "uViewPos",
    "uIsTextured",
    "uSampler0",
};

static constexpr std::array<osc::experimental::ShaderType, 7> g_ExpectedPropertyTypes =
{
    osc::experimental::ShaderType::Mat4,
    osc::experimental::ShaderType::Mat4,
    osc::experimental::ShaderType::Vec3,
    osc::experimental::ShaderType::Vec3,
    osc::experimental::ShaderType::Vec3,
    osc::experimental::ShaderType::Bool,
    osc::experimental::ShaderType::Sampler2D,
};

static_assert(g_ExpectedPropertyNames.size() == g_ExpectedPropertyTypes.size());

TEST(ShaderTypeTest, CanStreamShaderType)
{
    std::stringstream ss;
    ss << osc::experimental::ShaderType::Bool;

    ASSERT_EQ(ss.str(), "Bool");
}

TEST(ShaderTypeTest, CanConvertShaderTypeToString)
{
    std::string s = osc::experimental::to_string(osc::experimental::ShaderType::Bool);
    ASSERT_EQ(s, "Bool");
}

TEST(ShaderTypeTest, CanIterateAndStringStreamAllShaderTypes)
{
    for (int i = 0; i < static_cast<int>(osc::experimental::ShaderType::TOTAL); ++i)
    {
        // shouldn't crash - if it does then we've missed a case somewhere
        std::stringstream ss;
        ss << static_cast<osc::experimental::ShaderType>(i);
    }
}

TEST(ShaderTypeTest, CanIterateAndConvertAllShaderTypesToString)
{
    for (int i = 0; i < static_cast<int>(osc::experimental::ShaderType::TOTAL); ++i)
    {
        // shouldn't crash - if it does then we've missed a case somewhere
        std::string s = osc::experimental::to_string(static_cast<osc::experimental::ShaderType>(i));
    }
}

TEST_F(ShaderTest, CanConstructFromVertexShaderAndFragmentShader)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
}

TEST_F(ShaderTest, CanCopyConstructShader)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader copy{s};
}

TEST_F(ShaderTest, CanMoveConstructShader)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader copy{std::move(s)};
}

TEST_F(ShaderTest, CanCopyAssignShader)
{
    osc::experimental::Shader s1{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader s2{g_VertexShaderSrc, g_FragmentShaderSrc};

    s1 = s2;
}

TEST_F(ShaderTest, CanMoveAssignShader)
{
    osc::experimental::Shader s1{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader s2{g_VertexShaderSrc, g_FragmentShaderSrc};

    s1 = std::move(s2);
}

TEST_F(ShaderTest, CopyConstructedShaderIsEqualToOriginal)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader copy{s};

    ASSERT_EQ(s, copy);
}

TEST_F(ShaderTest, TwoDifferentShadersAreNotEqual)
{
    osc::experimental::Shader s1{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader s2{g_VertexShaderSrc, g_FragmentShaderSrc};

    ASSERT_NE(s1, s2);
}

TEST_F(ShaderTest, CanPrintShaderDetailsToAStringStream)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    std::stringstream ss;
    ss << s;  // shouldn't throw etc.

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(ShaderTest, CanWriteShaderAsStdString)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    std::string str = osc::experimental::to_string(s);

    ASSERT_FALSE(str.empty());
}

TEST_F(ShaderTest, StringRepresentationContainsExpectedInfo)
{
    // this test is flakey, but is just ensuring that the string printout has enough information
    // to help debugging etc.

    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    std::string str = osc::experimental::to_string(s);

    for (auto const& propName : g_ExpectedPropertyNames)
    {
        ASSERT_TRUE(osc::ContainsSubstring(str, std::string{propName}));
    }
}

TEST_F(ShaderTest, FindPropertyIndexCanFindAllExpectedProperties)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    for (auto const& propName : g_ExpectedPropertyNames)
    {
        ASSERT_TRUE(s.findPropertyIndex(std::string{propName}));
    }
}
TEST_F(ShaderTest, CompiledShaderHasExpectedNumberOfProperties)
{
    // (effectively, number of properties == number of uniforms)
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    ASSERT_EQ(s.getPropertyCount(), g_ExpectedPropertyNames.size());
}

TEST_F(ShaderTest, IteratingOverPropertyIndicesForNameReturnsValidPropertyName)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    std::unordered_set<std::string> allPropNames(g_ExpectedPropertyNames.begin(), g_ExpectedPropertyNames.end());
    std::unordered_set<std::string> returnedPropNames;

    for (int i = 0, len = s.getPropertyCount(); i < len; ++i)
    {
        returnedPropNames.insert(s.getPropertyName(i));
    }

    ASSERT_EQ(allPropNames, returnedPropNames);
}

TEST_F(ShaderTest, GetPropertyNameReturnsGivenPropertyName)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    for (auto const& propName : g_ExpectedPropertyNames)
    {
        std::optional<int> idx = s.findPropertyIndex(std::string{propName});
        ASSERT_TRUE(idx);
        ASSERT_EQ(s.getPropertyName(*idx), propName);
    }
}

TEST_F(ShaderTest, GetPropertyTypeReturnsExpectedType)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    for (int i = 0; i < g_ExpectedPropertyNames.size(); ++i)
    {
        static_assert(g_ExpectedPropertyNames.size() == g_ExpectedPropertyTypes.size());
        auto const& propName = g_ExpectedPropertyNames[i];
        osc::experimental::ShaderType expectedType = g_ExpectedPropertyTypes[i];

        std::optional<int> idx = s.findPropertyIndex(std::string{propName});
        ASSERT_TRUE(idx);
        ASSERT_EQ(s.getPropertyType(*idx), expectedType);
    }
}

TEST_F(ShaderTest, CanHashShader)
{
    // e.g. for storage in a set

    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    std::hash<osc::experimental::Shader>{}(s); // should compile and run fine
}