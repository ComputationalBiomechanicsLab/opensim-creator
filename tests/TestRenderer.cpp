#include "src/Graphics/Color.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Geometry.hpp"
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

static constexpr char const g_GeometryShaderSrc[] =
R"(
    #version 330 core

    layout (triangles) in;
    layout (line_strip, max_vertices = 6) out;

    void main()
    {
        gl_Position = gl_in[0].gl_Position;
        EmitVertex();
        gl_Position = gl_in[1].gl_Position;
        EmitVertex();
        gl_Position = gl_in[2].gl_Position;
        EmitVertex();
        EndPrimitive();
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

static osc::experimental::Texture2D GenerateTexture()
{
    std::vector<osc::Rgba32> pixels(4);
    return osc::experimental::Texture2D{2, 2, pixels};
}

static osc::experimental::Material GenerateMaterial()
{
    osc::experimental::Shader shader{g_VertexShaderSrc, g_FragmentShaderSrc};
    return osc::experimental::Material{shader};
}

static std::vector<glm::vec3> GenerateTriangleVerts()
{
    std::vector<glm::vec3> rv;
    for (int i = 0; i < 30; ++i)
    {
        rv.push_back(GenerateVec3());
    }
    return rv;
}

static osc::experimental::RenderTexture GenerateRenderTexture()
{
    osc::experimental::RenderTextureDescriptor d{2, 2};
    return osc::experimental::RenderTexture{d};
}

template<typename T>
static bool SpansEqual(nonstd::span<T const> a, nonstd::span<T const> b)
{
    if (a.size() != b.size())
    {
        return false;
    }

    for (int i = 0; i < a.size(); ++i)
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
    ss << osc::experimental::ShaderType::Bool;

    ASSERT_EQ(ss.str(), "Bool");
}

TEST_F(Renderer, ShaderTypeCanBeIteratedOverAndAllCanBeStreamed)
{
    for (int i = 0; i < static_cast<int>(osc::experimental::ShaderType::TOTAL); ++i)
    {
        // shouldn't crash - if it does then we've missed a case somewhere
        std::stringstream ss;
        ss << static_cast<osc::experimental::ShaderType>(i);
    }
}

TEST_F(Renderer, ShaderCanBeConstructedFromVertexAndFragmentShaderSource)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
}

TEST_F(Renderer, ShaderCanBeConstructedFromVertexGeometryAndFragmentShaderSources)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_GeometryShaderSrc, g_FragmentShaderSrc};
}

TEST_F(Renderer, ShaderCanBeCopyConstructed)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader copy{s};
}

TEST_F(Renderer, ShaderCanBeMoveConstructed)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader copy{std::move(s)};
}

TEST_F(Renderer, ShaderCanBeCopyAssigned)
{
    osc::experimental::Shader s1{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader s2{g_VertexShaderSrc, g_FragmentShaderSrc};

    s1 = s2;
}

TEST_F(Renderer, ShaderCanBeMoveAssigned)
{
    osc::experimental::Shader s1{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader s2{g_VertexShaderSrc, g_FragmentShaderSrc};

    s1 = std::move(s2);
}

TEST_F(Renderer, ShaderThatIsCopyConstructedEqualsSrcShader)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader copy{s};

    ASSERT_EQ(s, copy);
}

TEST_F(Renderer, ShadersThatDifferCompareNotEqual)
{
    osc::experimental::Shader s1{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Shader s2{g_VertexShaderSrc, g_FragmentShaderSrc};

    ASSERT_NE(s1, s2);
}

TEST_F(Renderer, ShaderCanBeWrittenToOutputStream)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    std::stringstream ss;
    ss << s;  // shouldn't throw etc.

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(Renderer, ShaderOutputStreamContainsExpectedInfo)
{
    // this test is flakey, but is just ensuring that the string printout has enough information
    // to help debugging etc.

    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

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
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    for (auto const& propName : g_ExpectedPropertyNames)
    {
        ASSERT_TRUE(s.findPropertyIndex(std::string{propName}));
    }
}
TEST_F(Renderer, ShaderHasExpectedNumberOfProperties)
{
    // (effectively, number of properties == number of uniforms)
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};
    ASSERT_EQ(s.getPropertyCount(), g_ExpectedPropertyNames.size());
}

TEST_F(Renderer, ShaderIteratingOverPropertyIndicesForNameReturnsValidPropertyName)
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

TEST_F(Renderer, ShaderGetPropertyNameReturnsGivenPropertyName)
{
    osc::experimental::Shader s{g_VertexShaderSrc, g_FragmentShaderSrc};

    for (auto const& propName : g_ExpectedPropertyNames)
    {
        std::optional<int> idx = s.findPropertyIndex(std::string{propName});
        ASSERT_TRUE(idx);
        ASSERT_EQ(s.getPropertyName(*idx), propName);
    }
}

TEST_F(Renderer, ShaderGetPropertyNameStillWorksIfTheUniformIsAnArray)
{
    osc::experimental::Shader s{g_VertexShaderWithArray, g_FragmentShaderWithArray};
    ASSERT_FALSE(s.findPropertyIndex("uFragColor[0]").has_value()) << "shouldn't expose 'raw' name";
    ASSERT_TRUE(s.findPropertyIndex("uFragColor").has_value()) << "should work, because the backend should normalize array-like uniforms to the original name (not uFragColor[0])";
}

TEST_F(Renderer, ShaderGetPropertyTypeReturnsExpectedType)
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

TEST_F(Renderer, MaterialCanBeConstructed)
{
    GenerateMaterial();  // should compile and run fine
}

TEST_F(Renderer, MaterialCanBeCopyConstructed)
{
    osc::experimental::Material material = GenerateMaterial();
    osc::experimental::Material copy{material};
}

TEST_F(Renderer, MaterialCanBeMoveConstructed)
{
    osc::experimental::Material material = GenerateMaterial();
    osc::experimental::Material copy{std::move(material)};
}

TEST_F(Renderer, MaterialCanBeCopyAssigned)
{
    osc::experimental::Material m1 = GenerateMaterial();
    osc::experimental::Material m2 = GenerateMaterial();

    m1 = m2;
}

TEST_F(Renderer, MaterialCanBeMoveAssigned)
{
    osc::experimental::Material m1 = GenerateMaterial();
    osc::experimental::Material m2 = GenerateMaterial();

    m1 = std::move(m2);
}

TEST_F(Renderer, MaterialThatIsCopyConstructedEqualsSourceMaterial)
{
    osc::experimental::Material material = GenerateMaterial();
    osc::experimental::Material copy{material};

    ASSERT_EQ(material, copy);
}

TEST_F(Renderer, MaterialThatIsCopyAssignedEqualsSourceMaterial)
{
    osc::experimental::Material m1 = GenerateMaterial();
    osc::experimental::Material m2 = GenerateMaterial();

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST_F(Renderer, MaterialGetShaderReturnsSuppliedShader)
{
    osc::experimental::Shader shader{g_VertexShaderSrc, g_FragmentShaderSrc};
    osc::experimental::Material material{shader};

    ASSERT_EQ(material.getShader(), shader);
}

TEST_F(Renderer, MaterialGetFloatOnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getFloat("someKey"));
}

TEST_F(Renderer, MaterialGetFloatArrayOnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getFloatArray("someKey"));
}

TEST_F(Renderer, MaterialGetVec3ArrayOnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec3Array("someKey"));
}

TEST_F(Renderer, MaterialGetVec3OnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec3("someKey"));
}

TEST_F(Renderer, MaterialGetVec4OnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec4("someKey"));
}

TEST_F(Renderer, MaterialGetMat3OnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getMat3("someKey"));
}

TEST_F(Renderer, MaterialGetMat4OnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getMat4("someKey"));
}

TEST_F(Renderer, MaterialGetIntOnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getInt("someKey"));
}

TEST_F(Renderer, MaterialGetBoolOnNewMaterialReturnsEmptyOptional)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getBool("someKey"));
}

TEST_F(Renderer, MaterialSetFloatOnMaterialCausesGetFloatToReturnTheProvidedValue)
{
    osc::experimental::Material mat = GenerateMaterial();

    std::string key = "someKey";
    float value = GenerateFloat();

    mat.setFloat(key, value);

    ASSERT_EQ(*mat.getFloat(key), value);
}

TEST_F(Renderer, MaterialSetFloatArrayOnMaterialCausesGetFloatArrayToReturnTheProvidedValues)
{
    osc::experimental::Material mat = GenerateMaterial();
    std::string key = "someKey";
    std::array<float, 4> values = {GenerateFloat(), GenerateFloat(), GenerateFloat(), GenerateFloat()};

    ASSERT_FALSE(mat.getFloatArray(key));

    mat.setFloatArray(key, values);

    nonstd::span<float const> rv = *mat.getFloatArray(key);
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(Renderer, MaterialSetVec3OnMaterialCausesGetVec3ToReturnTheProvidedValue)
{
    osc::experimental::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::vec3 value = GenerateVec3();

    mat.setVec3(key, value);

    ASSERT_EQ(*mat.getVec3(key), value);
}

TEST_F(Renderer, MaterialSetVec3ArrayOnMaterialCausesGetVec3ArrayToReutrnTheProvidedValues)
{
    osc::experimental::Material mat = GenerateMaterial();
    std::string key = "someKey";
    std::array<glm::vec3, 4> values = {GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3()};

    ASSERT_FALSE(mat.getVec3Array(key));

    mat.setVec3Array(key, values);

    nonstd::span<glm::vec3 const> rv = *mat.getVec3Array(key);
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(Renderer, MaterialSetVec4OnMaterialCausesGetVec4ToReturnTheProvidedValue)
{
    osc::experimental::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::vec4 value = GenerateVec4();

    mat.setVec4(key, value);

    ASSERT_EQ(*mat.getVec4(key), value);
}

TEST_F(Renderer, MaterialSetMat3OnMaterialCausesGetMat3ToReturnTheProvidedValue)
{
    osc::experimental::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::mat3 value = GenerateMat3x3();

    mat.setMat3(key, value);

    ASSERT_EQ(*mat.getMat3(key), value);
}

TEST_F(Renderer, MaterialSetMat4OnMaterialCausesGetMat4ToReturnTheProvidedValue)
{
    osc::experimental::Material mat = GenerateMaterial();

    std::string key = "someKey";
    glm::mat4 value = GenerateMat4x4();

    mat.setMat4(key, value);

    ASSERT_EQ(*mat.getMat4(key), value);
}

TEST_F(Renderer, MaterialSetIntOnMaterialCausesGetIntToReturnTheProvidedValue)
{
    osc::experimental::Material mat = GenerateMaterial();

    std::string key = "someKey";
    int value = GenerateInt();

    mat.setInt(key, value);

    ASSERT_EQ(*mat.getInt(key), value);
}

TEST_F(Renderer, MaterialSetBoolOnMaterialCausesGetBoolToReturnTheProvidedValue)
{
    osc::experimental::Material mat = GenerateMaterial();

    std::string key = "someKey";
    bool value = GenerateBool();

    mat.setBool(key, value);

    ASSERT_EQ(*mat.getBool(key), value);
}

TEST_F(Renderer, MaterialSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    osc::experimental::Material mat = GenerateMaterial();

    std::string key = "someKey";
    osc::experimental::Texture2D t = GenerateTexture();

    ASSERT_FALSE(mat.getTexture(key));

    mat.setTexture(key, t);

    ASSERT_TRUE(mat.getTexture(key));
}

TEST_F(Renderer, MaterialSetRenderTextureCausesGetRenderTextureToReturnTheTexture)
{
    osc::experimental::Material mat = GenerateMaterial();
    osc::experimental::RenderTexture renderTex = GenerateRenderTexture();
    std::string key = "someKey";

    ASSERT_FALSE(mat.getRenderTexture(key));

    mat.setRenderTexture(key, renderTex);

    ASSERT_EQ(*mat.getRenderTexture(key), renderTex);
}

TEST_F(Renderer, MaterialSetRenderTextureFollowedByClearRenderTextureClearsTheRenderTexture)
{
    osc::experimental::Material mat = GenerateMaterial();
    osc::experimental::RenderTexture renderTex = GenerateRenderTexture();
    std::string key = "someKey";

    ASSERT_FALSE(mat.getRenderTexture(key));

    mat.setRenderTexture(key, renderTex);

    ASSERT_EQ(*mat.getRenderTexture(key), renderTex);

    mat.clearRenderTexture(key);

    ASSERT_FALSE(mat.getRenderTexture(key));
}

TEST_F(Renderer, MaterialGetTransparentIsInitially)
{
    osc::experimental::Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getTransparent());
}

TEST_F(Renderer, MaterialSetTransparentBehavesAsExpected)
{
    osc::experimental::Material mat = GenerateMaterial();
    mat.setTransparent(true);
    ASSERT_TRUE(mat.getTransparent());
    mat.setTransparent(false);
    ASSERT_FALSE(mat.getTransparent());
    mat.setTransparent(true);
    ASSERT_TRUE(mat.getTransparent());
}

TEST_F(Renderer, MaterialCanCompareEquals)
{
    osc::experimental::Material mat = GenerateMaterial();
    osc::experimental::Material copy{mat};

    ASSERT_EQ(mat, copy);
}

TEST_F(Renderer, MaterialCanCompareNotEquals)
{
    osc::experimental::Material m1 = GenerateMaterial();
    osc::experimental::Material m2 = GenerateMaterial();

    ASSERT_NE(m1, m2);
}

TEST_F(Renderer, MaterialCanCompareLessThan)
{
    osc::experimental::Material m1 = GenerateMaterial();
    osc::experimental::Material m2 = GenerateMaterial();

    m1 < m2;  // should compile and not throw, but no guarantees about ordering
}

TEST_F(Renderer, MaterialCanPrintToStringStream)
{
    osc::experimental::Material m1 = GenerateMaterial();

    std::stringstream ss;
    ss << m1;
}

TEST_F(Renderer, MaterialOutputStringContainsUsefulInformation)
{
    osc::experimental::Material m1 = GenerateMaterial();
    std::stringstream ss;

    ss << m1;

    std::string str{ss.str()};

    ASSERT_TRUE(osc::ContainsSubstringCaseInsensitive(str, "Material"));

    // TODO: should print more useful info, such as number of props etc.
}

TEST_F(Renderer, MaterialSetFloatAndThenSetVec3CausesGetFloatToReturnEmpty)
{
    // compound test: when the caller sets a Vec3 then calling getInt with the same key should return empty
    osc::experimental::Material mat = GenerateMaterial();

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
    osc::experimental::MaterialPropertyBlock mpb;
}

TEST_F(Renderer, MaterialPropertyBlockCanCopyConstruct)
{
    osc::experimental::MaterialPropertyBlock mpb;
    osc::experimental::MaterialPropertyBlock copy{mpb};
}

TEST_F(Renderer, MaterialPropertyBlockCanMoveConstruct)
{
    osc::experimental::MaterialPropertyBlock mpb;
    osc::experimental::MaterialPropertyBlock copy{std::move(mpb)};
}

TEST_F(Renderer, MaterialPropertyBlockCanCopyAssign)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 = m2;
}

TEST_F(Renderer, MaterialPropertyBlockCanMoveAssign)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 = std::move(m2);
}

TEST_F(Renderer, MaterialPropertyBlock)
{
    osc::experimental::MaterialPropertyBlock mpb;

    ASSERT_TRUE(mpb.isEmpty());
}

TEST_F(Renderer, MaterialPropertyBlockCanClearDefaultConstructed)
{
    osc::experimental::MaterialPropertyBlock mpb;
    mpb.clear();

    ASSERT_TRUE(mpb.isEmpty());
}

TEST_F(Renderer, MaterialPropertyBlockClearClearsProperties)
{
    osc::experimental::MaterialPropertyBlock mpb;

    mpb.setFloat("someKey", GenerateFloat());

    ASSERT_FALSE(mpb.isEmpty());

    mpb.clear();

    ASSERT_TRUE(mpb.isEmpty());
}

TEST_F(Renderer, MaterialPropertyBlockGetFloatReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getFloat("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetVec3ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getVec3("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetVec4ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getVec4("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetMat3ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getMat3("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetMat4ReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getMat4("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetIntReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getInt("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetBoolReturnsEmptyOnDefaultConstructedInstance)
{
    osc::experimental::MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getBool("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockSetFloatCausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    float value = GenerateFloat();

    ASSERT_FALSE(mpb.getFloat(key));

    mpb.setFloat(key, value);
    ASSERT_TRUE(mpb.getFloat(key));
    ASSERT_EQ(mpb.getFloat(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetVec3CausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::vec3 value = GenerateVec3();

    ASSERT_FALSE(mpb.getVec3(key));

    mpb.setVec3(key, value);
    ASSERT_TRUE(mpb.getVec3(key));
    ASSERT_EQ(mpb.getVec3(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetVec4CausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::vec4 value = GenerateVec4();

    ASSERT_FALSE(mpb.getVec4(key));

    mpb.setVec4(key, value);
    ASSERT_TRUE(mpb.getVec4(key));
    ASSERT_EQ(mpb.getVec4(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetMat3CausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    glm::mat3 value = GenerateMat3x3();

    ASSERT_FALSE(mpb.getVec4(key));

    mpb.setMat3(key, value);
    ASSERT_TRUE(mpb.getMat3(key));
    ASSERT_EQ(mpb.getMat3(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetIntCausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    int value = GenerateInt();

    ASSERT_FALSE(mpb.getInt(key));

    mpb.setInt(key, value);
    ASSERT_TRUE(mpb.getInt(key));
    ASSERT_EQ(mpb.getInt(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetBoolCausesGetterToReturnSetValue)
{
    osc::experimental::MaterialPropertyBlock mpb;
    std::string key = "someKey";
    bool value = GenerateBool();

    ASSERT_FALSE(mpb.getBool(key));

    mpb.setBool(key, value);
    ASSERT_TRUE(mpb.getBool(key));
    ASSERT_EQ(mpb.getBool(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    osc::experimental::MaterialPropertyBlock mpb;

    std::string key = "someKey";
    osc::experimental::Texture2D t = GenerateTexture();

    ASSERT_FALSE(mpb.getTexture(key));

    mpb.setTexture(key, t);

    ASSERT_TRUE(mpb.getTexture(key));
}

TEST_F(Renderer, MaterialPropertyBlockCanCompareEquals)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 == m2;
}

TEST_F(Renderer, MaterialPropertyBlockCopyConstructionComparesEqual)
{
    osc::experimental::MaterialPropertyBlock m;
    osc::experimental::MaterialPropertyBlock copy{m};

    ASSERT_EQ(m, copy);
}

TEST_F(Renderer, MaterialPropertyBlockCopyAssignmentComparesEqual)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1.setFloat("someKey", GenerateFloat());

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST_F(Renderer, MaterialPropertyBlockDifferentMaterialBlocksCompareNotEqual)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1.setFloat("someKey", GenerateFloat());

    ASSERT_NE(m1, m2);
}

TEST_F(Renderer, MaterialPropertyBlockCanCompareLessThan)
{
    osc::experimental::MaterialPropertyBlock m1;
    osc::experimental::MaterialPropertyBlock m2;

    m1 < m2;  // just ensure this compiles and runs
}

TEST_F(Renderer, MaterialPropertyBlockCanPrintToOutputStream)
{
    osc::experimental::MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;  // just ensure this compiles and runs
}

TEST_F(Renderer, MaterialPropertyBlockPrintingToOutputStreamMentionsMaterialPropertyBlock)
{
    osc::experimental::MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;

    ASSERT_TRUE(osc::ContainsSubstring(ss.str(), "MaterialPropertyBlock"));
}

TEST_F(Renderer, TextureCanConstructFromRGBAPixels)
{
    std::vector<osc::Rgba32> pixels(4);
    osc::experimental::Texture2D t{2, 2, pixels};
}

TEST_F(Renderer, TextureRGBAThrowsIfDimensionsDontMatchNumberOfPixels)
{
    std::vector<osc::Rgba32> pixels(4);
    ASSERT_ANY_THROW({ osc::experimental::Texture2D t(1, 2, pixels); });
}

TEST_F(Renderer, TextureCanConstructFromSingleChannelPixels)
{
    std::vector<std::uint8_t> pixels(4);
    osc::experimental::Texture2D t{2, 2, pixels};
}

TEST_F(Renderer, TextureSingleChannelConstructorThrowsIfDimensionsDoesNotMatchNumberOfPixels)
{
    std::vector<std::uint8_t> pixels(4);
    ASSERT_ANY_THROW({ osc::experimental::Texture2D t(2, 1, pixels); });
}

TEST_F(Renderer, TextureSingleChannelConstructedReturnsCorrectValuesOnGetters)
{
    std::vector<std::uint8_t> pixels(4);
    osc::experimental::Texture2D t{2, 2, pixels};

    ASSERT_EQ(t.getWidth(), 2);
    ASSERT_EQ(t.getHeight(), 2);
    ASSERT_EQ(t.getAspectRatio(), 1.0f);
}

TEST_F(Renderer, TextureWithRuntimeNumberOfChannelsWorksForSingleChannelData)
{
    std::vector<std::uint8_t> singleChannelPixels(16);
    osc::experimental::Texture2D t{4, 4, singleChannelPixels, 1};

    ASSERT_EQ(t.getWidth(), 4);
    ASSERT_EQ(t.getHeight(), 4);
    ASSERT_EQ(t.getAspectRatio(), 1.0f);
}

TEST_F(Renderer, TextureWithRuntimeNumberOfChannelsWorksForRGBData)
{
    std::vector<std::uint8_t> rgbPixels(12);
    osc::experimental::Texture2D t{2, 2, rgbPixels, 3};

    ASSERT_EQ(t.getWidth(), 2);
    ASSERT_EQ(t.getHeight(), 2);
    ASSERT_EQ(t.getAspectRatio(), 1.0f);
}

TEST_F(Renderer, TextureWithRuntimeNumberOfChannelsWorksForRGBAData)
{
    std::vector<std::uint8_t> rgbaPixels(16);
    osc::experimental::Texture2D t{2, 2, rgbaPixels, 4};

    ASSERT_EQ(t.getWidth(), 2);
    ASSERT_EQ(t.getHeight(), 2);
    ASSERT_EQ(t.getAspectRatio(), 1.0f);
}

TEST_F(Renderer, TextureWith2NumberOfChannelsThrowsException)
{
    std::vector<std::uint8_t> weirdPixels(8);
    ASSERT_ANY_THROW({ osc::experimental::Texture2D t(2, 2, weirdPixels, 2); });
}

TEST_F(Renderer, TextureWith5ChannelsThrowsException)
{
    std::vector<std::uint8_t> weirdPixels(20);
    ASSERT_ANY_THROW({ osc::experimental::Texture2D t(2, 2, weirdPixels, 5); });
}

TEST_F(Renderer, TextureCanCopyConstruct)
{
    osc::experimental::Texture2D t = GenerateTexture();
    osc::experimental::Texture2D copy{t};
}

TEST_F(Renderer, TextureCanMoveConstruct)
{
    osc::experimental::Texture2D t = GenerateTexture();
    osc::experimental::Texture2D copy{std::move(t)};
}

TEST_F(Renderer, TextureCanCopyAssign)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 = t2;
}

TEST_F(Renderer, TextureCanMoveAssign)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 = std::move(t2);
}

TEST_F(Renderer, TextureGetWidthReturnsSuppliedWidth)
{
    int width = 2;
    int height = 6;
    std::vector<osc::Rgba32> pixels(width*height);

    osc::experimental::Texture2D t{width, height, pixels};

    ASSERT_EQ(t.getWidth(), width);
}

TEST_F(Renderer, TextureGetHeightReturnsSuppliedHeight)
{
    int width = 2;
    int height = 6;
    std::vector<osc::Rgba32> pixels(width*height);

    osc::experimental::Texture2D t{width, height, pixels};

    ASSERT_EQ(t.getHeight(), height);
}

TEST_F(Renderer, TextureGetAspectRatioReturnsExpectedRatio)
{
    int width = 16;
    int height = 37;
    std::vector<osc::Rgba32> pixels(width*height);

    osc::experimental::Texture2D t{width, height, pixels};

    float expected = static_cast<float>(width) / static_cast<float>(height);

    ASSERT_FLOAT_EQ(t.getAspectRatio(), expected);
}

TEST_F(Renderer, TextureGetWrapModeReturnsRepeatedByDefault)
{
    osc::experimental::Texture2D t = GenerateTexture();
    ASSERT_EQ(t.getWrapMode(), osc::experimental::TextureWrapMode::Repeat);
}

TEST_F(Renderer, TextureSetWrapModeMakesSubsequentGetWrapModeReturnNewWrapMode)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapMode(), wm);

    t.setWrapMode(wm);

    ASSERT_EQ(t.getWrapMode(), wm);
}

TEST_F(Renderer, TextureSetWrapModeCausesGetWrapModeUToAlsoReturnNewWrapMode)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapMode(), wm);
    ASSERT_NE(t.getWrapModeU(), wm);

    t.setWrapMode(wm);

    ASSERT_EQ(t.getWrapModeU(), wm);
}

TEST_F(Renderer, TextureSetWrapModeUCausesGetWrapModeUToReturnValue)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeU(), wm);

    t.setWrapModeU(wm);

    ASSERT_EQ(t.getWrapModeU(), wm);
}

TEST_F(Renderer, TextureSetWrapModeVCausesGetWrapModeVToReturnValue)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeV(), wm);

    t.setWrapModeV(wm);

    ASSERT_EQ(t.getWrapModeV(), wm);
}

TEST_F(Renderer, TextureSetWrapModeWCausesGetWrapModeWToReturnValue)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeW(), wm);

    t.setWrapModeW(wm);

    ASSERT_EQ(t.getWrapModeW(), wm);
}

TEST_F(Renderer, TextureSetFilterModeCausesGetFilterModeToReturnValue)
{
    osc::experimental::Texture2D t = GenerateTexture();

    osc::experimental::TextureFilterMode tfm = osc::experimental::TextureFilterMode::Linear;

    ASSERT_NE(t.getFilterMode(), tfm);

    t.setFilterMode(tfm);

    ASSERT_EQ(t.getFilterMode(), tfm);
}

TEST_F(Renderer, TextureCanBeComparedForEquality)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 == t2;  // just ensure it compiles + runs
}

TEST_F(Renderer, TextureCopyConstructingComparesEqual)
{
    osc::experimental::Texture2D t = GenerateTexture();
    osc::experimental::Texture2D tcopy{t};

    ASSERT_EQ(t, tcopy);
}

TEST_F(Renderer, TextureCopyAssignmentMakesEqualityReturnTrue)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 = t2;

    ASSERT_EQ(t1, t2);
}

TEST_F(Renderer, TextureCanBeComparedForNotEquals)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 != t2;
}

TEST_F(Renderer, TextureChangingWrapModeMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapMode(), wm);

    t2.setWrapMode(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingWrapModeUMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeU(), wm);

    t2.setWrapModeU(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingWrapModeVMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeV(), wm);

    t2.setWrapModeV(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingWrapModeWMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureWrapMode wm = osc::experimental::TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeW(), wm);

    t2.setWrapModeW(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingFilterModeMakesCopyUnequal)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2{t1};
    osc::experimental::TextureFilterMode fm = osc::experimental::TextureFilterMode::Linear;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getFilterMode(), fm);

    t2.setFilterMode(fm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureCanBeComparedLessThan)
{
    osc::experimental::Texture2D t1 = GenerateTexture();
    osc::experimental::Texture2D t2 = GenerateTexture();

    t1 < t2;  // just ensure it compiles + runs
}

TEST_F(Renderer, TextureCanBeWrittenToOutputStream)
{
    osc::experimental::Texture2D t = GenerateTexture();

    std::stringstream ss;
    ss << t;

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(Renderer, MeshTopographyAllCanBeWrittenToStream)
{
    for (int i = 0; i < static_cast<int>(osc::experimental::MeshTopography::TOTAL); ++i)
    {
        osc::experimental::MeshTopography mt = static_cast<osc::experimental::MeshTopography>(i);

        std::stringstream ss;

        ss << mt;

        ASSERT_FALSE(ss.str().empty());
    }
}

TEST_F(Renderer, LoadTexture2DFromImageResourceCanLoadImageFile)
{
    osc::experimental::Texture2D t = osc::experimental::LoadTexture2DFromImageResource("awesomeface.png");
    ASSERT_EQ(t.getWidth(), 512);
    ASSERT_EQ(t.getHeight(), 512);
}

TEST_F(Renderer, LoadTexture2DFromImageResourceThrowsIfResourceNotFound)
{
    ASSERT_ANY_THROW({ osc::experimental::LoadTexture2DFromImageResource("doesnt_exist.png"); });
}

TEST_F(Renderer, MeshCanBeDefaultConstructed)
{
    osc::experimental::Mesh mesh;
}

TEST_F(Renderer, MeshCanBeCopyConstructed)
{
    osc::experimental::Mesh m;
    osc::experimental::Mesh copy{m};
}

TEST_F(Renderer, MeshCanBeMoveConstructed)
{
    osc::experimental::Mesh m1;
    osc::experimental::Mesh m2{std::move(m1)};
}

TEST_F(Renderer, MeshCanBeCopyAssigned)
{
    osc::experimental::Mesh m1;
    osc::experimental::Mesh m2;

    m1 = m2;
}

TEST_F(Renderer, MeshCanMeMoveAssigned)
{
    osc::experimental::Mesh m1;
    osc::experimental::Mesh m2;

    m1 = std::move(m2);
}

TEST_F(Renderer, MeshCanGetTopography)
{
    osc::experimental::Mesh m;

    m.getTopography();
}

TEST_F(Renderer, MeshGetTopographyDefaultsToTriangles)
{
    osc::experimental::Mesh m;

    ASSERT_EQ(m.getTopography(), osc::experimental::MeshTopography::Triangles);
}

TEST_F(Renderer, MeshSetTopographyCausesGetTopographyToUseSetValue)
{
    osc::experimental::Mesh m;
    osc::experimental::MeshTopography topography = osc::experimental::MeshTopography::Lines;

    ASSERT_NE(m.getTopography(), osc::experimental::MeshTopography::Lines);

    m.setTopography(topography);

    ASSERT_EQ(m.getTopography(), topography);
}

TEST_F(Renderer, MeshSetTopographyCausesCopiedMeshTobeNotEqualToInitialMesh)
{
    osc::experimental::Mesh m;
    osc::experimental::Mesh copy{m};
    osc::experimental::MeshTopography topography = osc::experimental::MeshTopography::Lines;

    ASSERT_EQ(m, copy);
    ASSERT_NE(copy.getTopography(), topography);

    copy.setTopography(topography);

    ASSERT_NE(m, copy);
}

TEST_F(Renderer, MeshGetVertsReturnsEmptyVertsOnDefaultConstruction)
{
    osc::experimental::Mesh m;
    ASSERT_TRUE(m.getVerts().empty());
}

TEST_F(Renderer, MeshSetVertsMakesGetCallReturnVerts)
{
    osc::experimental::Mesh m;
    std::vector<glm::vec3> verts = GenerateTriangleVerts();

    ASSERT_FALSE(SpansEqual(m.getVerts(), nonstd::span<glm::vec3 const>(verts)));
}

TEST_F(Renderer, MeshSetVertsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    osc::experimental::Mesh m;
    osc::experimental::Mesh copy{m};

    ASSERT_EQ(m, copy);

    copy.setVerts(GenerateTriangleVerts());

    ASSERT_NE(m, copy);
}

TEST_F(Renderer, MeshGetNormalsReturnsEmptyOnDefaultConstruction)
{
    osc::experimental::Mesh m;
    ASSERT_TRUE(m.getNormals().empty());
}

TEST_F(Renderer, MeshSetNormalsMakesGetCallReturnSuppliedData)
{
    osc::experimental::Mesh m;
    std::vector<glm::vec3> normals = {GenerateVec3(), GenerateVec3(), GenerateVec3()};

    ASSERT_TRUE(m.getNormals().empty());

    m.setNormals(normals);

    ASSERT_TRUE(SpansEqual(m.getNormals(), nonstd::span<glm::vec3 const>(normals)));
}

TEST_F(Renderer, MeshSetNormalsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    osc::experimental::Mesh m;
    osc::experimental::Mesh copy{m};
    std::vector<glm::vec3> normals = {GenerateVec3(), GenerateVec3(), GenerateVec3()};

    ASSERT_EQ(m, copy);

    copy.setNormals(normals);

    ASSERT_NE(m, copy);
}

TEST_F(Renderer, MeshGetTexCoordsReturnsEmptyOnDefaultConstruction)
{
    osc::experimental::Mesh m;
    ASSERT_TRUE(m.getTexCoords().empty());
}

TEST_F(Renderer, MeshSetTexCoordsCausesGetToReturnSuppliedData)
{
    osc::experimental::Mesh m;
    std::vector<glm::vec2> coords = {GenerateVec2(), GenerateVec2(), GenerateVec2()};

    ASSERT_TRUE(m.getTexCoords().empty());

    m.setTexCoords(coords);

    ASSERT_TRUE(SpansEqual(m.getTexCoords(), nonstd::span<glm::vec2 const>(coords)));
}

TEST_F(Renderer, MeshSetTexCoordsCausesCopiedMeshToNotBeEqualToInitialMesh)
{
    osc::experimental::Mesh m;
    osc::experimental::Mesh copy{m};
    std::vector<glm::vec2> coords = {GenerateVec2(), GenerateVec2(), GenerateVec2()};

    ASSERT_EQ(m, copy);

    copy.setTexCoords(coords);

    ASSERT_NE(m, copy);
}

TEST_F(Renderer, MeshGetColorsInitiallyReturnsEmptySpan)
{
    osc::experimental::Mesh m;
    ASSERT_TRUE(m.getColors().empty());
}

TEST_F(Renderer, MeshSetColorsFollowedByGetColorsReturnsColors)
{
    osc::experimental::Mesh m;
    std::array<osc::Rgba32, 3> colors{};

    m.setColors(colors);

    auto rv = m.getColors();
    ASSERT_EQ(rv.size(), colors.size());
}

TEST_F(Renderer, MeshGetNumIndicesReturnsZeroOnDefaultConstruction)
{
    osc::experimental::Mesh m;
    ASSERT_EQ(m.getNumIndices(), 0);
}

TEST_F(Renderer, MeshGetBoundsReturnsEmptyBoundsOnInitialization)
{
    osc::experimental::Mesh m;
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

    osc::experimental::Mesh m;
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
        { 0.0f,  0.0f, 1.0f},  // tip
    };
    std::uint16_t pyramidIndices[] = {0, 1, 2, 3};

    osc::experimental::Mesh m;
    m.setVerts(pyramid);
    m.setIndices(pyramidIndices);
    osc::AABB expected = osc::AABBFromVerts(pyramid);
    ASSERT_EQ(m.getBounds(), expected);
}

TEST_F(Renderer, MeshGetMidpointReturnsZeroVecOnInitialization)
{
    osc::experimental::Mesh m;
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
        { 0.0f,  0.0f, 1.0f},  // tip
    };
    std::uint16_t pyramidIndices[] = {0, 1, 2, 3};

    osc::experimental::Mesh m;
    m.setVerts(pyramid);
    m.setIndices(pyramidIndices);
    glm::vec3 expected = osc::Midpoint(osc::AABBFromVerts(pyramid));
    ASSERT_EQ(m.getMidpoint(), expected);
}

TEST_F(Renderer, MeshCanBeComparedForEquality)
{
    osc::experimental::Mesh m1;
    osc::experimental::Mesh m2;

    m1 == m2;
}

TEST_F(Renderer, MeshCopiesAreEqual)
{
    osc::experimental::Mesh m;
    osc::experimental::Mesh copy{m};

    ASSERT_EQ(m, copy);
}

TEST_F(Renderer, MeshCanBeComparedForNotEquals)
{
    osc::experimental::Mesh m1;
    osc::experimental::Mesh m2;

    m1 != m2;
}

TEST_F(Renderer, MeshCanBeComparedLessThan)
{
    osc::experimental::Mesh m1;
    osc::experimental::Mesh m2;

    m1 < m2;
}

TEST_F(Renderer, MeshCanBeWrittenToOutputStreamForDebugging)
{
    osc::experimental::Mesh m;
    std::stringstream ss;

    ss << m;

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(Renderer, LoadMeshFromMeshDataWorksAsExpected)
{
    osc::MeshData cube = osc::GenCube();
    osc::experimental::Mesh mesh = osc::experimental::LoadMeshFromMeshData(cube);

    ASSERT_EQ(mesh.getTopography(), osc::experimental::MeshTopography::Triangles);
    ASSERT_TRUE(mesh.getColors().empty());
    nonstd::span<glm::vec3 const> verts = mesh.getVerts();
    ASSERT_TRUE(std::equal(verts.begin(), verts.end(), cube.verts.begin(), cube.verts.end()));
    nonstd::span<glm::vec3 const> normals = mesh.getNormals();
    ASSERT_TRUE(std::equal(normals.begin(), normals.end(), cube.normals.begin(), cube.normals.end()));
    nonstd::span<uint32_t> indices = mesh.getIndices();
    ASSERT_TRUE(std::equal(indices.begin(), indices.end(), cube.indices.begin(), cube.indices.end()));
    nonstd::span<glm::vec2 const> coords = mesh.getTexCoords();
    ASSERT_TRUE(std::equal(coords.begin(), coords.end(), cube.texcoords.begin(), cube.texcoords.end()));
}

TEST_F(Renderer, LoadMeshFromMeshDataAlsoObeysTheMeshDatasTopography)
{
    osc::MeshData cube = osc::GenCubeLines();
    osc::experimental::Mesh mesh = osc::experimental::LoadMeshFromMeshData(cube);
    ASSERT_EQ(mesh.getTopography(), osc::experimental::MeshTopography::Lines);
}

TEST_F(Renderer, LoadMeshFromLegacyMeshWorksAsExpected)
{
    osc::MeshData data = osc::GenCube();
    osc::Mesh legacyMesh{data};
    osc::experimental::Mesh convertedMesh = osc::experimental::LoadMeshFromLegacyMesh(legacyMesh);

    ASSERT_EQ(convertedMesh.getTopography(), osc::experimental::MeshTopography::Triangles);
    ASSERT_TRUE(SpansEqual(legacyMesh.getVerts(), convertedMesh.getVerts()));
    ASSERT_TRUE(SpansEqual(legacyMesh.getNormals(), convertedMesh.getNormals()));
    ASSERT_TRUE(SpansEqual(legacyMesh.getTexCoords(), convertedMesh.getTexCoords()));
    ASSERT_EQ(legacyMesh.getIndices(), convertedMesh.getIndices());
    ASSERT_EQ(legacyMesh.getAABB(), convertedMesh.getBounds());
}

TEST_F(Renderer, RenderTextureFormatCanBeIteratedOverAndStreamedToString)
{
    for (int i = 0; i < static_cast<int>(osc::experimental::RenderTextureFormat::TOTAL); ++i)
    {
        std::stringstream ss;
        ss << static_cast<osc::experimental::RenderTextureFormat>(i);  // shouldn't throw
    }
}

TEST_F(Renderer, DepthStencilFormatCanBeIteratedOverAndStreamedToString)
{
    for (int i = 0; i < static_cast<int>(osc::experimental::DepthStencilFormat::TOTAL); ++i)
    {
        std::stringstream ss;
        ss << static_cast<osc::experimental::DepthStencilFormat>(i);  // shouldn't throw
    }
}

TEST_F(Renderer, RenderTextureDescriptorCanBeConstructedFromWithAndHeight)
{
    osc::experimental::RenderTextureDescriptor d{1, 1};
}

TEST_F(Renderer, RenderTextureDescriptorThrowsIfGivenNegativeWidth)
{
    ASSERT_ANY_THROW({ osc::experimental::RenderTextureDescriptor d(-1, 1); });
}

TEST_F(Renderer, RenderTextureDescriptorThrowsIfGivenNegativeHeight)
{
    ASSERT_ANY_THROW({ osc::experimental::RenderTextureDescriptor d(1, -1); });
}

TEST_F(Renderer, RenderTextureDescriptorCanBeCopyConstructed)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{d1};
}

TEST_F(Renderer, RenderTextureDescriptorCanBeMoveConstructed)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{std::move(d1)};
}

TEST_F(Renderer, RenderTextureDescriptorCanBeCopyAssigned)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{1, 1};
    d1 = d2;
}

TEST_F(Renderer, RenderTextureDescriptorCanBeMoveAssigned)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{1, 1};
    d1 = std::move(d2);
}

TEST_F(Renderer, RenderTextureDescriptorGetWidthReturnsConstructedWith)
{
    int width = 1;
    osc::experimental::RenderTextureDescriptor d1{width, 1};
    ASSERT_EQ(d1.getWidth(), width);
}

TEST_F(Renderer, RenderTextureDescriptorSetWithFollowedByGetWithReturnsSetWidth)
{
    int newWidth = 31;
    osc::experimental::RenderTextureDescriptor d1{1, 1};

    d1.setWidth(newWidth);
    ASSERT_EQ(d1.getWidth(), newWidth);
}

TEST_F(Renderer, RenderTextureDescriptorSetWidthNegativeValueThrows)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    ASSERT_ANY_THROW({ d1.setWidth(-1); });
}

TEST_F(Renderer, RenderTextureDescriptorGetHeightReturnsConstructedHeight)
{
    int height = 1;
    osc::experimental::RenderTextureDescriptor d1{1, height};
    ASSERT_EQ(d1.getHeight(), height);
}

TEST_F(Renderer, RenderTextureDescriptorSetHeightFollowedByGetHeightReturnsSetHeight)
{
    int newHeight = 31;
    osc::experimental::RenderTextureDescriptor d1{1, 1};

    d1.setHeight(newHeight);
    ASSERT_EQ(d1.getHeight(), newHeight);
}

TEST_F(Renderer, RenderTextureDescriptorGetAntialiasingLevelInitiallyReturns1)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    ASSERT_EQ(d1.getAntialiasingLevel(), 1);
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelMakesGetAntialiasingLevelReturnValue)
{
    int newAntialiasingLevel = 4;

    osc::experimental::RenderTextureDescriptor d1{1, 1};
    d1.setAntialiasingLevel(newAntialiasingLevel);
    ASSERT_EQ(d1.getAntialiasingLevel(), newAntialiasingLevel);
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelToZeroThrows)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    ASSERT_ANY_THROW({ d1.setAntialiasingLevel(0); });
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingNegativeThrows)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    ASSERT_ANY_THROW({ d1.setAntialiasingLevel(-1); });
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelToInvalidValueThrows)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    ASSERT_ANY_THROW({ d1.setAntialiasingLevel(3); });
}

TEST_F(Renderer, RenderTextureDescriptorGetColorFormatReturnsARGB32ByDefault)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    ASSERT_EQ(d1.getColorFormat(), osc::experimental::RenderTextureFormat::ARGB32);
}

TEST_F(Renderer, RenderTextureDescriptorGetDepthStencilFormatReturnsDefaultValue)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    ASSERT_EQ(d1.getDepthStencilFormat(), osc::experimental::DepthStencilFormat::D24_UNorm_S8_UInt);
}

TEST_F(Renderer, RenderTextureDescriptorComparesEqualOnCopyConstruct)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{d1};

    ASSERT_EQ(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorComparesEqualWithSameConstructionVals)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{1, 1};

    ASSERT_EQ(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorSetWithMakesItCompareNotEqual)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{1, 1};

    d2.setWidth(2);

    ASSERT_NE(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorSetHeightMakesItCompareNotEqual)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{1, 1};

    d2.setHeight(2);

    ASSERT_NE(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelMakesItCompareNotEqual)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{1, 1};

    d2.setAntialiasingLevel(2);

    ASSERT_NE(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorSetAntialiasingLevelToSameValueComparesEqual)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTextureDescriptor d2{1, 1};

    d2.setAntialiasingLevel(d2.getAntialiasingLevel());

    ASSERT_EQ(d1, d2);
}

TEST_F(Renderer, RenderTextureDescriptorCanBeStreamedToAString)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    std::stringstream ss;
    ss << d1;

    std::string str{ss.str()};
    ASSERT_TRUE(osc::ContainsSubstringCaseInsensitive(str, "RenderTextureDescriptor"));
}

TEST_F(Renderer, RenderTextureCanBeConstructedFromADescriptor)
{
    osc::experimental::RenderTextureDescriptor d1{1, 1};
    osc::experimental::RenderTexture d{d1};
}

TEST_F(Renderer, RenderTextureFromDescriptorHasExpectedValues)
{
    int width = 5;
    int height = 8;
    int aaLevel = 4;

    osc::experimental::RenderTextureDescriptor desc{width, height};
    desc.setAntialiasingLevel(aaLevel);

    osc::experimental::RenderTexture tex{desc};

    ASSERT_EQ(tex.getWidth(), width);
    ASSERT_EQ(tex.getHeight(), height);
    ASSERT_EQ(tex.getAntialiasingLevel(), aaLevel);
}

TEST_F(Renderer, EmplaceOrReformatWorksAsExpected)
{
    std::optional<osc::experimental::RenderTexture> opt;

    osc::experimental::RenderTextureDescriptor desc1{5, 6};

    ASSERT_FALSE(opt.has_value());

    osc::experimental::EmplaceOrReformat(opt, desc1);

    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(opt->getWidth(), desc1.getWidth());
    ASSERT_EQ(opt->getHeight(), desc1.getHeight());

    osc::experimental::RenderTextureDescriptor desc2{7, 8};

    osc::experimental::EmplaceOrReformat(opt, desc2);

    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(opt->getWidth(), desc2.getWidth());
    ASSERT_EQ(opt->getHeight(), desc2.getHeight());
}

TEST_F(Renderer, CameraProjectionCanBeStreamed)
{
    for (int i = 0; i < static_cast<int>(osc::experimental::CameraProjection::TOTAL); ++i)
    {
        std::stringstream ss;
        ss << static_cast<osc::experimental::CameraProjection>(i);

        ASSERT_FALSE(ss.str().empty());
    }
}

TEST_F(Renderer, CameraCanDefaultConstruct)
{
    osc::experimental::Camera camera;  // should compile + run
}

TEST_F(Renderer, CameraDefaultConstructedHasNoTexture)
{
    osc::experimental::Camera camera;
    ASSERT_FALSE(camera.getTexture().has_value());
}

TEST_F(Renderer, CameraCanBeConstructedWithTexture)
{
    osc::experimental::Camera camera{GenerateRenderTexture()};  // should compile + run
}

TEST_F(Renderer, CameraConstructedWithTextureMakesGetTextureReturnNonemptyOptional)
{
    osc::experimental::Camera camera{GenerateRenderTexture()};
    ASSERT_TRUE(camera.getTexture().has_value());
}

TEST_F(Renderer, CameraConstructedWithTextureMakesGetTextureReturnTextureWithSameWidthAndHeight)
{
    osc::experimental::RenderTexture t = GenerateRenderTexture();
    osc::experimental::Camera camera{t};

    ASSERT_EQ(t.getWidth(), camera.getTexture()->getWidth());
    ASSERT_EQ(t.getHeight(), camera.getTexture()->getHeight());
}

TEST_F(Renderer, CameraCanBeCopyConstructed)
{
    osc::experimental::Camera c;
    osc::experimental::Camera copy{c};
}

TEST_F(Renderer, CameraThatIsCopyConstructedComparesEqual)
{
    osc::experimental::Camera c;
    osc::experimental::Camera copy{c};

    ASSERT_EQ(c, copy);
}

TEST_F(Renderer, CameraCanBeMoveConstructed)
{
    osc::experimental::Camera c;
    osc::experimental::Camera copy{std::move(c)};
}

TEST_F(Renderer, CameraCanBeCopyAssigned)
{
    osc::experimental::Camera c1;
    osc::experimental::Camera c2;

    c2 = c1;
}

TEST_F(Renderer, CameraThatIsCopyAssignedComparesEqualToSource)
{
    osc::experimental::Camera c1;
    osc::experimental::Camera c2;

    c1 = c2;

    ASSERT_EQ(c1, c2);
}

TEST_F(Renderer, CameraCanBeMoveAssigned)
{
    osc::experimental::Camera c1;
    osc::experimental::Camera c2;

    c2 = std::move(c1);
}

TEST_F(Renderer, CameraCanGetBackgroundColor)
{
    osc::experimental::Camera camera;

    ASSERT_EQ(camera.getBackgroundColor(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
}

TEST_F(Renderer, CameraCanSetBackgroundColor)
{
    osc::experimental::Camera camera;
    camera.setBackgroundColor(GenerateVec4());
}

TEST_F(Renderer, CameraSetBackgroundColorMakesGetBackgroundColorReturnTheColor)
{
    osc::experimental::Camera camera;
    glm::vec4 color = GenerateVec4();

    camera.setBackgroundColor(color);

    ASSERT_EQ(camera.getBackgroundColor(), color);
}

TEST_F(Renderer, CameraSetBackgroundColorMakesCameraCompareNonEqualWithCopySource)
{
    osc::experimental::Camera camera;
    osc::experimental::Camera copy{camera};

    ASSERT_EQ(camera, copy);

    copy.setBackgroundColor(GenerateVec4());

    ASSERT_NE(camera, copy);
}

TEST_F(Renderer, CameraGetCameraProjectionReturnsProject)
{
    osc::experimental::Camera camera;
    ASSERT_EQ(camera.getCameraProjection(), osc::experimental::CameraProjection::Perspective);
}

TEST_F(Renderer, CameraCanSetCameraProjection)
{
    osc::experimental::Camera camera;
    camera.setCameraProjection(osc::experimental::CameraProjection::Orthographic);
}

TEST_F(Renderer, CameraSetCameraProjectionMakesGetCameraProjectionReturnSetProjection)
{
    osc::experimental::Camera camera;
    osc::experimental::CameraProjection proj = osc::experimental::CameraProjection::Orthographic;

    ASSERT_NE(camera.getCameraProjection(), proj);

    camera.setCameraProjection(proj);

    ASSERT_EQ(camera.getCameraProjection(), proj);
}

TEST_F(Renderer, CameraSetCameraProjectionMakesCameraCompareNotEqual)
{
    osc::experimental::Camera camera;
    osc::experimental::Camera copy{camera};
    osc::experimental::CameraProjection proj = osc::experimental::CameraProjection::Orthographic;

    ASSERT_NE(copy.getCameraProjection(), proj);

    copy.setCameraProjection(proj);

    ASSERT_NE(camera, copy);
}

TEST_F(Renderer, CameraGetViewMatrixReturnsViewMatrixBasedOnPositonDirectionAndUp)
{
    osc::experimental::Camera camera;
    camera.setCameraProjection(osc::experimental::CameraProjection::Orthographic);
    camera.setPosition({0.0f, 0.0f, 0.0f});

    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 expectedMatrix{1.0f};

    ASSERT_EQ(viewMatrix, expectedMatrix);
}

TEST_F(Renderer, CameraSetViewMatrixSetsANewViewMatrixThatCanBeRetrievedWithGetViewMatrix)
{
    osc::experimental::Camera camera;

    // these shouldn't matter - they're overridden
    camera.setCameraProjection(osc::experimental::CameraProjection::Orthographic);
    camera.setPosition({7.0f, 5.0f, -3.0f});  

    glm::mat4 viewMatrix{1.0f};
    viewMatrix[0][1] = 9.0f;  // change some part of it

    camera.setViewMatrix(viewMatrix);

    ASSERT_EQ(camera.getViewMatrix(), viewMatrix);
}

TEST_F(Renderer, CameraResetViewMatrixResetsTheViewMatrixToUsingStandardCameraPositionEtc)
{
    osc::experimental::Camera camera;
    glm::mat4 initialViewMatrix = camera.getViewMatrix();

    glm::mat4 viewMatrix{1.0f};
    viewMatrix[0][1] = 9.0f;  // change some part of it

    camera.setViewMatrix(viewMatrix);
    ASSERT_NE(camera.getViewMatrix(), initialViewMatrix);
    ASSERT_EQ(camera.getViewMatrix(), viewMatrix);

    camera.resetViewMatrix();

    ASSERT_EQ(camera.getViewMatrix(), initialViewMatrix);
}

TEST_F(Renderer, CameraGetProjectionMatrixReturnsProjectionMatrixBasedOnPositonDirectionAndUp)
{
    osc::experimental::Camera camera;
    camera.setCameraProjection(osc::experimental::CameraProjection::Orthographic);
    camera.setPosition({0.0f, 0.0f, 0.0f});

    glm::mat4 mtx = camera.getProjectionMatrix();
    glm::mat4 expected{1.0f};

    // only compare the Y, Z, and W columns: the X column depends on the aspect ratio of the output
    // target
    ASSERT_EQ(mtx[1], expected[1]);
    ASSERT_EQ(mtx[2], expected[2]);
    ASSERT_EQ(mtx[3], expected[3]);
}

TEST_F(Renderer, CameraSetProjectionMatrixSetsANewProjectionMatrixThatCanBeRetrievedWithGetProjectionMatrix)
{
    osc::experimental::Camera camera;

    // these shouldn't matter - they're overridden
    camera.setCameraProjection(osc::experimental::CameraProjection::Orthographic);
    camera.setPosition({7.0f, 5.0f, -3.0f});  

    glm::mat4 ProjectionMatrix{1.0f};
    ProjectionMatrix[0][1] = 9.0f;  // change some part of it

    camera.setProjectionMatrix(ProjectionMatrix);

    ASSERT_EQ(camera.getProjectionMatrix(), ProjectionMatrix);
}

TEST_F(Renderer, CameraResetProjectionMatrixResetsTheProjectionMatrixToUsingStandardCameraPositionEtc)
{
    osc::experimental::Camera camera;
    glm::mat4 initialProjectionMatrix = camera.getProjectionMatrix();

    glm::mat4 ProjectionMatrix{1.0f};
    ProjectionMatrix[0][1] = 9.0f;  // change some part of it

    camera.setProjectionMatrix(ProjectionMatrix);
    ASSERT_NE(camera.getProjectionMatrix(), initialProjectionMatrix);
    ASSERT_EQ(camera.getProjectionMatrix(), ProjectionMatrix);

    camera.resetProjectionMatrix();

    ASSERT_EQ(camera.getProjectionMatrix(), initialProjectionMatrix);
}


// TODO MeshSetIndicesU16CausesGetNumIndicesToEqualSuppliedNumberOfIndices
// TODO Mesh::getIndices
// TODO Mesh::setIndices U16
// TODO Mesh::setIndices U32
// TODO Mesh ensure > 2^16 indices are allowed
// TODO Mesh::clear
// 
// TODO: RenderTexture (all)

// TODO: Texture: ensure texture debug string contains useful information etc.

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
