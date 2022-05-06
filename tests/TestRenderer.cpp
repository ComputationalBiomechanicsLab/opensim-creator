#include "src/Graphics/Renderer.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/CStringView.hpp"

#include <gtest/gtest.h>

#include <array>
#include <random>
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

// helper for material tests
static osc::experimental::Material CreateMaterial()
{
    osc::experimental::Shader shader{g_VertexShaderSrc, g_FragmentShaderSrc};
    return osc::experimental::Material{shader};
}

TEST_F(ShaderTest, CanConstructMaterial)
{
    CreateMaterial();  // should compile and run fine
}

TEST_F(ShaderTest, CanCopyConstructMaterial)
{
    osc::experimental::Material material = CreateMaterial();
    osc::experimental::Material copy{material};
}

TEST_F(ShaderTest, CanMoveConstructMaterial)
{
    osc::experimental::Material material = CreateMaterial();
    osc::experimental::Material copy{std::move(material)};
}

TEST_F(ShaderTest, CanCopyAssignMaterial)
{
    osc::experimental::Material m1 = CreateMaterial();
    osc::experimental::Material m2 = CreateMaterial();

    m1 = m2;
}

TEST_F(ShaderTest, CanMoveAssignMaterial)
{
    osc::experimental::Material m1 = CreateMaterial();
    osc::experimental::Material m2 = CreateMaterial();

    m1 = std::move(m2);
}

TEST_F(ShaderTest, CopyConstructedMaterialComparesEqualWithCopiedFromMaterial)
{
    osc::experimental::Material material = CreateMaterial();
    osc::experimental::Material copy{material};

    ASSERT_EQ(material, copy);
}

TEST_F(ShaderTest, CopyAssignedMaterialComparesEqualWithCopiedFromMaterial)
{
    osc::experimental::Material m1 = CreateMaterial();
    osc::experimental::Material m2 = CreateMaterial();

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST_F(ShaderTest, GetShaderReturnsShaderSuppliedInCtor)
{
    osc::experimental::Shader shader{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Material material{shader};

    ASSERT_EQ(material.getShader(), shader);
}

TEST_F(ShaderTest, GetFloatOnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = CreateMaterial();
    ASSERT_FALSE(mat.getFloat("someKey"));
}

TEST_F(ShaderTest, GetVec3OnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = CreateMaterial();
    ASSERT_FALSE(mat.getVec3("someKey"));
}

TEST_F(ShaderTest, GetVec4OnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = CreateMaterial();
    ASSERT_FALSE(mat.getVec4("someKey"));
}

TEST_F(ShaderTest, GetMat3OnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = CreateMaterial();
    ASSERT_FALSE(mat.getMat3("someKey"));
}

TEST_F(ShaderTest, GetMat4OnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = CreateMaterial();
    ASSERT_FALSE(mat.getMat4("someKey"));
}

TEST_F(ShaderTest, GetMat4x3OnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = CreateMaterial();
    ASSERT_FALSE(mat.getMat4x3("someKey"));
}

TEST_F(ShaderTest, GetIntOnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = CreateMaterial();
    ASSERT_FALSE(mat.getInt("someKey"));
}

TEST_F(ShaderTest, GetBoolOnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = CreateMaterial();
    ASSERT_FALSE(mat.getBool("someKey"));
}

TEST_F(ShaderTest, SetFloatOnMaterialCausesGetFloatToReturnTheProvidedValue)
{
    osc::experimental::Material mat = CreateMaterial();

    std::string key = "someKey";
    float value = GenerateFloat();

    mat.setFloat(key, value);

    ASSERT_EQ(*mat.getFloat(key), value);
}

TEST_F(ShaderTest, SetVec3OnMaterialCausesGetVec3ToReturnTheProvidedValue)
{
    osc::experimental::Material mat = CreateMaterial();

    std::string key = "someKey";
    glm::vec3 value = GenerateVec3();

    mat.setVec3(key, value);

    ASSERT_EQ(*mat.getVec3(key), value);
}

TEST_F(ShaderTest, SetVec4OnMaterialCausesGetVec4ToReturnTheProvidedValue)
{
    osc::experimental::Material mat = CreateMaterial();

    std::string key = "someKey";
    glm::vec4 value = GenerateVec4();

    mat.setVec4(key, value);

    ASSERT_EQ(*mat.getVec4(key), value);
}

TEST_F(ShaderTest, SetMat3OnMaterialCausesGetMat3ToReturnTheProvidedValue)
{
    osc::experimental::Material mat = CreateMaterial();

    std::string key = "someKey";
    glm::mat3 value = GenerateMat3x3();

    mat.setMat3(key, value);

    ASSERT_EQ(*mat.getMat3(key), value);
}

TEST_F(ShaderTest, SetMat4OnMaterialCausesGetMat4ToReturnTheProvidedValue)
{
    osc::experimental::Material mat = CreateMaterial();

    std::string key = "someKey";
    glm::mat4 value = GenerateMat4x4();

    mat.setMat4(key, value);

    ASSERT_EQ(*mat.getMat4(key), value);
}

TEST_F(ShaderTest, SetMat4x3OnMaterialCausesGetMat4x3ToReturnTheProvidedValue)
{
    osc::experimental::Material mat = CreateMaterial();

    std::string key = "someKey";
    glm::mat4x3 value = GenerateMat4x3();

    mat.setMat4x3(key, value);

    ASSERT_EQ(*mat.getMat4x3(key), value);
}

TEST_F(ShaderTest, SetIntOnMaterialCausesGetIntToReturnTheProvidedValue)
{
    osc::experimental::Material mat = CreateMaterial();

    std::string key = "someKey";
    int value = GenerateInt();

    mat.setInt(key, value);

    ASSERT_EQ(*mat.getInt(key), value);
}

TEST_F(ShaderTest, SetBoolOnMaterialCausesGetBoolToReturnTheProvidedValue)
{
    osc::experimental::Material mat = CreateMaterial();

    std::string key = "someKey";
    bool value = GenerateBool();

    mat.setBool(key, value);

    ASSERT_EQ(*mat.getBool(key), value);
}

TEST_F(ShaderTest, CanCompareEquals)
{
    osc::experimental::Material mat = CreateMaterial();
    osc::experimental::Material copy{mat};

    ASSERT_EQ(mat, copy);
}

TEST_F(ShaderTest, CanCompareNotEquals)
{
    osc::experimental::Material m1 = CreateMaterial();
    osc::experimental::Material m2 = CreateMaterial();

    ASSERT_NE(m1, m2);
}

TEST_F(ShaderTest, CanCompareLessThan)
{
    osc::experimental::Material m1 = CreateMaterial();
    osc::experimental::Material m2 = CreateMaterial();

    m1 < m2;  // should compile and not throw, but no guarantees about ordering
}

TEST_F(ShaderTest, CanCompareLessThanOrEqualsTo)
{
    osc::experimental::Material m1 = CreateMaterial();
    osc::experimental::Material m2 = CreateMaterial();

    m1 <= m2;  // should compile and not throw, but no guarantees about ordering
}

TEST_F(ShaderTest, CanCompareGreaterThan)
{
    osc::experimental::Material m1 = CreateMaterial();
    osc::experimental::Material m2 = CreateMaterial();

    m1 > m2;  // should compile and not throw, but no guarantees about ordering
}

TEST_F(ShaderTest, CanCompareGreaterThanOrEqualsTo)
{
    osc::experimental::Material m1 = CreateMaterial();
    osc::experimental::Material m2 = CreateMaterial();

    m1 >= m2;  // should compile and not throw, but no guarantees about ordering
}

TEST_F(ShaderTest, CanPrintToStringStream)
{
    osc::experimental::Material m1 = CreateMaterial();

    std::stringstream ss;
    ss << m1;
}

// TODO: test print contains relevant strings etc.

TEST_F(ShaderTest, CanConvertToString)
{
    osc::experimental::Material m1 = CreateMaterial();
    std::string s = osc::experimental::to_string(m1);
}

TEST_F(ShaderTest, CanHash)
{
    osc::experimental::Material m1 = CreateMaterial();
    std::hash<osc::experimental::Material>{}(m1);
}

// TODO: compound tests: ensure copy on write works etc

TEST_F(ShaderTest, SetFloatAndThenSetVec3CausesGetFloatToReturnEmpty)
{
    // compound test: when the caller sets a Vec3 then calling getInt with the same key should return empty
    osc::experimental::Material mat = CreateMaterial();

    std::string key = "someKey";
    float floatValue = GenerateFloat();
    glm::vec3 vecValue = GenerateVec3();

    mat.setFloat(key, floatValue);

    ASSERT_TRUE(mat.getFloat(key));

    mat.setVec3(key, vecValue);

    ASSERT_TRUE(mat.getVec3(key));
    ASSERT_FALSE(mat.getFloat(key));
}
