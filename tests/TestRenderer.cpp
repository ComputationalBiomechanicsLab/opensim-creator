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

class TextureTest : public ::testing::Test {
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

    std::unordered_set<std::string> allPropNames;
    for (auto const& sv : g_ExpectedPropertyNames)
    {
        allPropNames.insert(std::string{sv});
    }
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

TEST(MaterialPropertyBlockTest, CanDefaultConstruct)
{
    osc::experimental::MaterialPropertyBlock mpb;
}

TEST(MaterialPropertyBlockTest, CanCopyConstruct)
{
    osc::experimental::MaterialPropertyBlock mpb;
    osc::experimental::MaterialPropertyBlock copy{mpb};
}

TEST(MaterialPropertyBlockTest, CanMoveConstruct)
{
    osc::experimental::MaterialPropertyBlock mpb;
    osc::experimental::MaterialPropertyBlock copy{std::move(mpb)};
}

TEST(MaterialPropertyBlockTest, CanCopyAssign)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 = m2;
}

TEST(MaterialPropertyBlockTest, CanMoveAssign)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 = std::move(m2);
}

TEST(MaterialPropertyBlocKTest, DefaultConstructedIsEmpty)
{
    osc::experimental::MaterialPropertyBlock mpb;

    ASSERT_TRUE(mpb.isEmpty());
}

TEST(MaterialPropertyBlockTest, CanClearDefaultConstructed)
{
    osc::experimental::MaterialPropertyBlock mpb;
    mpb.clear();

    ASSERT_TRUE(mpb.isEmpty());
}

TEST(MaterialPropertyBlockTest, ClearClearsProperties)
{
    osc::experimental::MaterialPropertyBlock mpb;

    mpb.setFloat("someKey", GenerateFloat());

    ASSERT_FALSE(mpb.isEmpty());

    mpb.clear();

    ASSERT_TRUE(mpb.isEmpty());
}

TEST(MaterialPropertyBlockTest, GetFloatReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getFloat("someKey"));
}

TEST(MaterialPropertyBlockTest, GetVec3ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getVec3("someKey"));
}

TEST(MaterialPropertyBlockTest, GetVec4ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getVec4("someKey"));
}

TEST(MaterialPropertyBlockTest, GetMat3ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getMat3("someKey"));
}

TEST(MaterialPropertyBlockTest, GetMat4ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getMat4("someKey"));
}

TEST(MaterialPropertyBlockTest, GetMat4x3ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getMat4x3("someKey"));
}

TEST(MaterialPropertyBlockTest, GetIntReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getInt("someKey"));
}

TEST(MaterialPropertyBlockTest, GetBoolReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getBool("someKey"));
}

TEST(MaterialPropertyBlockTest, SetFloatCausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    float value = GenerateFloat();

    ASSERT_FALSE(mpb.getFloat(key));

    mpb.setFloat(key, value);
    ASSERT_TRUE(mpb.getFloat(key));
    ASSERT_EQ(mpb.getFloat(key), value);
}

TEST(MaterialPropertyBlockTest, SetVec3CausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::vec3 value = GenerateVec3();

    ASSERT_FALSE(mpb.getVec3(key));

    mpb.setVec3(key, value);
    ASSERT_TRUE(mpb.getVec3(key));
    ASSERT_EQ(mpb.getVec3(key), value);
}

TEST(MaterialPropertyBlockTest, SetVec4CausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::vec4 value = GenerateVec4();

    ASSERT_FALSE(mpb.getVec4(key));

    mpb.setVec4(key, value);
    ASSERT_TRUE(mpb.getVec4(key));
    ASSERT_EQ(mpb.getVec4(key), value);
}

TEST(MaterialPropertyBlockTest, SetMat3CausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::mat3 value = GenerateMat3x3();

    ASSERT_FALSE(mpb.getVec4(key));

    mpb.setMat3(key, value);
    ASSERT_TRUE(mpb.getMat3(key));
    ASSERT_EQ(mpb.getMat3(key), value);
}

TEST(MaterialPropertyBlockTest, SetMat4x3CausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::mat4x3 value = GenerateMat4x3();

    ASSERT_FALSE(mpb.getMat4x3(key));

    mpb.setMat4x3(key, value);
    ASSERT_TRUE(mpb.getMat4x3(key));
    ASSERT_EQ(mpb.getMat4x3(key), value);
}

TEST(MaterialPropertyBlockTest, SetIntCausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    int value = GenerateInt();

    ASSERT_FALSE(mpb.getInt(key));

    mpb.setInt(key, value);
    ASSERT_TRUE(mpb.getInt(key));
    ASSERT_EQ(mpb.getInt(key), value);
}

TEST(MaterialPropertyBlockTest, SetBoolCausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    bool value = GenerateBool();

    ASSERT_FALSE(mpb.getBool(key));

    mpb.setBool(key, value);
    ASSERT_TRUE(mpb.getBool(key));
    ASSERT_EQ(mpb.getBool(key), value);
}

TEST(MaterialPropertyBlockTest, CanCompareEquals)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 == m2;
}

TEST(MaterialPropertyBlockTest, CopyConstructionComparesEqual)
{
    osc::experimental::MaterialPropertyBlock m;
    osc::experimental::MaterialPropertyBlock copy{m};

    ASSERT_EQ(m, copy);
}

TEST(MaterialPropertyBlockTest, CopyAssignmentComparesEqual)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1.setFloat("someKey", GenerateFloat());

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST(MaterialPropertyBlockTest, DifferentMaterialBlocksCompareNotEqual)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1.setFloat("someKey", GenerateFloat());

    ASSERT_NE(m1, m2);
}

TEST(MaterialPropertyBlockTest, CanCompareLessThan)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 < m2;  // just ensure this compiles and runs
}

TEST(MaterialPropertyBlockTest, CanCompareLessThanOrEqualTo)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 <= m2;  // just ensure this compiles and runs
}

TEST(MaterialPropertyBlockTest, CanCompareGreaterThan)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 > m2;  // just ensure this compiles and runs
}

TEST(MaterialPropertyBlockTest, CanCompareGreaterThanOrEqualTo)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 >= m2;  // just ensure this compiles and runs
}

TEST(MaterialPropertyBlockTest, CanPrintToOutputStream)
{
    osc::experimental::MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;  // just ensure this compiles and runs
}

TEST(MaterialPropertyBlckTest, PrintingToOutputStreamMentionsMaterialPropertyBlock)
{
    osc::experimental::MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;

    ASSERT_TRUE(osc::ContainsSubstring(ss.str(), "MaterialPropertyBlock"));
}

TEST(MaterialPropertyBlockTest, CanConvertToString)
{
    osc::experimental::MaterialPropertyBlock m;

    std::string s = osc::experimental::to_string(m);  // just ensure this compiles + runs
}

TEST(MaterialPropertyBlockTest, CanHash)
{
    osc::experimental::MaterialPropertyBlock m;
    std::hash<osc::experimental::MaterialPropertyBlock>{}(m);
}

// TOOD: ensure string contains relevant stuff etc

// TODO: ensure printout mentions variables etc.

// TODO: compound test: set a float but read a vec, etc.

TEST_F(TextureTest, CanConstructFromPixels)
{
    std::vector<osc::Rgba32> pixels(4);
    osc::experimental::Texture2D t{2, 2, pixels};
}

TEST_F(TextureTest, ThrowsIfDimensionsDontMatchNumberOfPixels)
{
    std::vector<osc::Rgba32> pixels(4);
    ASSERT_ANY_THROW({ osc::experimental::Texture2D t(1, 2, pixels); });
}

static osc::experimental::Texture2D GenerateTexture()
{
    std::vector<osc::Rgba32> pixels(4);
    return osc::experimental::Texture2D{2, 2, pixels};
}

TEST_F(TextureTest, CanCopyConstruct)
{
    osc::experimental::Texture2D t = GenerateTexture();
    osc::experimental::Texture2D copy{t};
}

TEST_F(TextureTest, CanMoveConstruct)
{
    osc::experimental::Texture2D t = GenerateTexture();
    osc::experimental::Texture2D copy{std::move(t)};
}

TEST_F(TextureTest, CanCopyAssign)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 = t2;
}

TEST_F(TextureTest, CanMoveAssign)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 = std::move(t2);
}

TEST_F(TextureTest, GetWidthReturnsSuppliedWidth)
{
    int width = 2;
    int height = 6;
    std::vector<osc::Rgba32> pixels(width*height);

    osc::experimental::Texture2D t{width, height, pixels};

    ASSERT_EQ(t.getWidth(), width);
}

TEST_F(TextureTest, GetHeightReturnsSuppliedHeight)
{
    int width = 2;
    int height = 6;
    std::vector<osc::Rgba32> pixels(width*height);

    osc::experimental::Texture2D t{width, height, pixels};

    ASSERT_EQ(t.getHeight(), height);
}

TEST_F(TextureTest, GetAspectRatioReturnsExpectedRatio)
{
    int width = 16;
    int height = 37;
    std::vector<osc::Rgba32> pixels(width*height);

    osc::experimental::Texture2D t{width, height, pixels};

    float expected = static_cast<float>(width) / static_cast<float>(height);

    ASSERT_FLOAT_EQ(t.getAspectRatio(), expected);
}

TEST_F(TextureTest, GetWrapModeReturnsRepeatedByDefault)
{
    osc::experimental::Texture2D t = GenerateTexture();
    ASSERT_EQ(t.getWrapMode(), osc::experimental::TextureWrapMode::Repeat);
}

TEST_F(TextureTest, SetWrapModeMakesSubsequentGetWrapModeReturnNewWrapMode)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapMode(), wm);

    t.setWrapMode(wm);

    ASSERT_EQ(t.getWrapMode(), wm);
}

TEST_F(TextureTest, SetWrapModeCausesGetWrapModeUToAlsoReturnNewWrapMode)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapMode(), wm);
    ASSERT_NE(t.getWrapModeU(), wm);

    t.setWrapMode(wm);

    ASSERT_EQ(t.getWrapModeU(), wm);
}

TEST_F(TextureTest, SetWrapModeUCausesGetWrapModeUToReturnValue)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeU(), wm);

    t.setWrapModeU(wm);

    ASSERT_EQ(t.getWrapModeU(), wm);
}

TEST_F(TextureTest, SetWrapModeVCausesGetWrapModeVToReturnValue)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeV(), wm);

    t.setWrapModeV(wm);

    ASSERT_EQ(t.getWrapModeV(), wm);
}

TEST_F(TextureTest, SetWrapModeWCausesGetWrapModeWToReturnValue)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeW(), wm);

    t.setWrapModeW(wm);

    ASSERT_EQ(t.getWrapModeW(), wm);
}

TEST_F(TextureTest, SetFilterModeCausesGetFilterModeToReturnValue)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureFilterMode tfm = osc::experimental::TextureFilterMode::Linear;

    ASSERT_NE(t.getFilterMode(), tfm);

    t.setFilterMode(tfm);

    ASSERT_EQ(t.getFilterMode(), tfm);
}

TEST_F(TextureTest, CanBeComparedForEquality)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 == t2;  // just ensure it compiles + runs
}

TEST_F(TextureTest, CopyConstructingComparesEqual)
{
    osc::experimental::Texture2D t = GenerateTexture();
    osc::experimental::Texture2D tcopy{t};

    ASSERT_EQ(t, tcopy);
}

TEST_F(TextureTest, CopyAssignmentMakesEqualityReturnTrue)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 = t2;

    ASSERT_EQ(t1, t2);
}

TEST_F(TextureTest, CanBeComparedForNotEquals)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 != t2;
}

TEST_F(TextureTest, ChangingWrapModeMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapMode(), wm);

    t2.setWrapMode(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(TextureTest, ChangingWrapModeUMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeU(), wm);

    t2.setWrapModeU(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(TextureTest, ChangingWrapModeVMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeV(), wm);

    t2.setWrapModeV(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(TextureTest, ChangingWrapModeWMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeW(), wm);

    t2.setWrapModeW(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(TextureTest, ChangingFilterModeMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureFilterMode fm = osc::experimental::TextureFilterMode::Linear;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getFilterMode(), fm);

    t2.setFilterMode(fm);

    ASSERT_NE(t1, t2);
}

TEST_F(TextureTest, CanBeComparedLessThan)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 < t2;  // just ensure it compiles + runs
}

TEST_F(TextureTest, CanBeComparedLessThanOrEqualTo)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 <= t2;
}

TEST_F(TextureTest, CanBeComparedGreaterThan)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 > t2;
}

TEST_F(TextureTest, CanBeComparedGreaterThanOrEqualTo)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 >= t2;
}

TEST_F(TextureTest, CanBeWrittenToOutputStream)
{
    osc::experimental::Texture2D t = GenerateTexture();

    std::stringstream ss;
    ss << t;

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(TextureTest, CaneBeConvertedToString)
{
    osc::experimental::Texture2D t = GenerateTexture();

    std::string s = osc::experimental::to_string(t);

    ASSERT_FALSE(s.empty());
}

TEST_F(TextureTest, CanBeHashed)
{
    osc::experimental::Texture2D t = GenerateTexture();

    std::hash<osc::experimental::Texture2D>{}(t);
}

// TODO ensure texture debug string contains useful information etc.