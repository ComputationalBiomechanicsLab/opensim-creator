// multiple associated headers: this should be broken up into
// smaller pieces

#include <testoscar/testoscarconfig.hpp>
#include <testoscar/TestingHelpers.hpp>

#include <gtest/gtest.h>
#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/CameraClearFlags.hpp>
#include <oscar/Graphics/CameraProjection.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/Cubemap.hpp>
#include <oscar/Graphics/CullMode.hpp>
#include <oscar/Graphics/DepthStencilFormat.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/GraphicsContext.hpp>
#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/MaterialPropertyBlock.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/MeshGenerators.hpp>
#include <oscar/Graphics/MeshTopology.hpp>
#include <oscar/Graphics/RenderTexture.hpp>
#include <oscar/Graphics/RenderTextureDescriptor.hpp>
#include <oscar/Graphics/RenderTextureFormat.hpp>
#include <oscar/Graphics/RenderTextureReadWrite.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/ShaderPropertyType.hpp>
#include <oscar/Graphics/SubMeshDescriptor.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureFormat.hpp>
#include <oscar/Graphics/TextureWrapMode.hpp>
#include <oscar/Graphics/TextureFilterMode.hpp>
#include <oscar/Maths/AABB.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/Mat3.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Platform/AppMetadata.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/EnumHelpers.hpp>
#include <oscar/Utils/ObjectRepresentation.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>

using osc::testing::ContainersEqual;
using osc::testing::GenerateBool;
using osc::testing::GenerateFloat;
using osc::testing::GenerateInt;
using osc::testing::GenerateMat3x3;
using osc::testing::GenerateMat4x4;
using osc::testing::GenerateVec2;
using osc::testing::GenerateVec3;
using osc::testing::GenerateVec4;
using osc::AABB;
using osc::AntiAliasingLevel;
using osc::App;
using osc::AppMetadata;
using osc::Camera;
using osc::CameraClearFlags;
using osc::CameraProjection;
using osc::Color;
using osc::ColorSpace;
using osc::Color32;
using osc::Contains;
using osc::ContainsCaseInsensitive;
using osc::ContiguousContainer;
using osc::CStringView;
using osc::Cubemap;
using osc::CullMode;
using osc::DepthFunction;
using osc::DepthStencilFormat;
using osc::Dot;
using osc::Identity;
using osc::Material;
using osc::MaterialPropertyBlock;
using osc::Mat3;
using osc::Mat4;
using osc::Mesh;
using osc::MeshTopology;
using osc::Normalize;
using osc::NumOptions;
using osc::Quat;
using osc::RenderTexture;
using osc::RenderTextureDescriptor;
using osc::RenderTextureFormat;
using osc::RenderTextureReadWrite;
using osc::Shader;
using osc::ShaderPropertyType;
using osc::SubMeshDescriptor;
using osc::TextureDimensionality;
using osc::TextureFilterMode;
using osc::TextureFormat;
using osc::TextureWrapMode;
using osc::Texture2D;
using osc::Transform;
using osc::Vec2;
using osc::Vec2i;
using osc::Vec3;
using osc::Vec4;
using osc::ViewObjectRepresentations;

namespace
{
    std::unique_ptr<App> g_App;

    class Renderer : public ::testing::Test {
    protected:
        static void SetUpTestSuite()
        {
            AppMetadata metadata{TESTOSCAR_ORGNAME_STRING, TESTOSCAR_APPNAME_STRING};
            g_App = std::make_unique<App>(metadata);
        }

        static void TearDownTestSuite()
        {
            g_App.reset();
        }
    };

    constexpr CStringView c_VertexShaderSrc = R"(
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
            vec3 frag2lightDir = normalize(-uLightDir);  // light direction is in the opposite direction
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

    constexpr CStringView c_FragmentShaderSrc = R"(
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

    constexpr CStringView c_VertexShaderWithArray = R"(
        #version 330 core

        layout (location = 0) in vec3 aPos;

        void main()
        {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    constexpr CStringView c_FragmentShaderWithArray = R"(
        #version 330 core

        uniform vec4 uFragColor[3];

        out vec4 FragColor;

        void main()
        {
            FragColor = uFragColor[0];
        }
    )";

    // expected, based on the above shader code
    constexpr std::array<CStringView, 14> c_ExpectedPropertyNames = std::to_array<CStringView>(
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
    });

    constexpr std::array<ShaderPropertyType, 14> c_ExpectedPropertyTypes = std::to_array(
    {
        ShaderPropertyType::Mat4,
        ShaderPropertyType::Mat4,
        ShaderPropertyType::Vec3,
        ShaderPropertyType::Vec3,
        ShaderPropertyType::Float,
        ShaderPropertyType::Float,
        ShaderPropertyType::Float,
        ShaderPropertyType::Bool,
        ShaderPropertyType::Sampler2D,
        ShaderPropertyType::Float,
        ShaderPropertyType::Vec3,
        ShaderPropertyType::Vec4,
        ShaderPropertyType::Float,
        ShaderPropertyType::Float,
    });
    static_assert(c_ExpectedPropertyNames.size() == c_ExpectedPropertyTypes.size());

    constexpr CStringView c_GeometryShaderVertSrc = R"(
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

    constexpr CStringView c_GeometryShaderGeomSrc = R"(
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

    constexpr CStringView c_GeometryShaderFragSrc = R"(
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

    // from: https://learnopengl.com/Advanced-OpenGL/Cubemaps
    constexpr CStringView c_CubemapVertexShader = R"(
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

    constexpr CStringView c_CubemapFragmentShader = R"(
        #version 330 core

        out vec4 FragColor;

        in vec3 TexCoords;

        uniform samplerCube skybox;

        void main()
        {
            FragColor = texture(skybox, TexCoords);
        }
    )";

    Texture2D GenerateTexture()
    {
        Texture2D rv{Vec2i{2, 2}};
        rv.setPixels(std::vector<Color>(4, Color::red()));
        return rv;
    }

    Material GenerateMaterial()
    {
        Shader shader{c_VertexShaderSrc, c_FragmentShaderSrc};
        return Material{shader};
    }

    RenderTexture GenerateRenderTexture()
    {
        RenderTextureDescriptor d{{2, 2}};
        return RenderTexture{d};
    }
}


// TESTS

TEST_F(Renderer, ShaderTypeCanStreamToString)
{
    std::stringstream ss;
    ss << ShaderPropertyType::Bool;

    ASSERT_EQ(ss.str(), "Bool");
}

TEST_F(Renderer, ShaderTypeCanBeIteratedOverAndAllCanBeStreamed)
{
    for (size_t i = 0; i < NumOptions<ShaderPropertyType>(); ++i)
    {
        // shouldn't crash - if it does then we've missed a case somewhere
        std::stringstream ss;
        ss << static_cast<ShaderPropertyType>(i);
    }
}

TEST_F(Renderer, ShaderCanBeConstructedFromVertexAndFragmentShaderSource)
{
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};
}

TEST_F(Renderer, ShaderCanBeConstructedFromVertexGeometryAndFragmentShaderSources)
{
    Shader s{c_GeometryShaderVertSrc, c_GeometryShaderGeomSrc, c_GeometryShaderFragSrc};
}

TEST_F(Renderer, ShaderCanBeCopyConstructed)
{
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};
    Shader{s};
}

TEST_F(Renderer, ShaderCanBeMoveConstructed)
{
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};
    Shader copy{std::move(s)};
}

TEST_F(Renderer, ShaderCanBeCopyAssigned)
{
    Shader s1{c_VertexShaderSrc, c_FragmentShaderSrc};
    Shader s2{c_VertexShaderSrc, c_FragmentShaderSrc};

    s1 = s2;
}

TEST_F(Renderer, ShaderCanBeMoveAssigned)
{
    Shader s1{c_VertexShaderSrc, c_FragmentShaderSrc};
    Shader s2{c_VertexShaderSrc, c_FragmentShaderSrc};

    s1 = std::move(s2);
}

TEST_F(Renderer, ShaderThatIsCopyConstructedEqualsSrcShader)
{
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};
    Shader copy{s}; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(s, copy);
}

TEST_F(Renderer, ShadersThatDifferCompareNotEqual)
{
    Shader s1{c_VertexShaderSrc, c_FragmentShaderSrc};
    Shader s2{c_VertexShaderSrc, c_FragmentShaderSrc};

    ASSERT_NE(s1, s2);
}

TEST_F(Renderer, ShaderCanBeWrittenToOutputStream)
{
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};

    std::stringstream ss;
    ss << s;  // shouldn't throw etc.

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(Renderer, ShaderOutputStreamContainsExpectedInfo)
{
    // this test is flakey, but is just ensuring that the string printout has enough information
    // to help debugging etc.

    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};

    std::stringstream ss;
    ss << s;
    std::string str{std::move(ss).str()};

    for (auto const& propName : c_ExpectedPropertyNames)
    {
        ASSERT_TRUE(Contains(str, std::string{propName}));
    }
}

TEST_F(Renderer, ShaderFindPropertyIndexCanFindAllExpectedProperties)
{
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};

    for (auto const& propName : c_ExpectedPropertyNames)
    {
        ASSERT_TRUE(s.findPropertyIndex(std::string{propName}));
    }
}
TEST_F(Renderer, ShaderHasExpectedNumberOfProperties)
{
    // (effectively, number of properties == number of uniforms)
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};
    ASSERT_EQ(s.getPropertyCount(), c_ExpectedPropertyNames.size());
}

TEST_F(Renderer, ShaderIteratingOverPropertyIndicesForNameReturnsValidPropertyName)
{
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};

    std::unordered_set<std::string> allPropNames;
    for (auto const& sv : c_ExpectedPropertyNames)
    {
        allPropNames.insert(std::string{sv});
    }
    std::unordered_set<std::string> returnedPropNames;

    for (size_t i = 0, len = s.getPropertyCount(); i < len; ++i)
    {
        returnedPropNames.insert(std::string{s.getPropertyName(i)});
    }

    ASSERT_EQ(allPropNames, returnedPropNames);
}

TEST_F(Renderer, ShaderGetPropertyNameReturnsGivenPropertyName)
{
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};

    for (auto const& propName : c_ExpectedPropertyNames)
    {
        std::optional<size_t> idx = s.findPropertyIndex(propName);
        ASSERT_TRUE(idx);
        ASSERT_EQ(s.getPropertyName(*idx), propName);
    }
}

TEST_F(Renderer, ShaderGetPropertyNameStillWorksIfTheUniformIsAnArray)
{
    Shader s{c_VertexShaderWithArray, c_FragmentShaderWithArray};
    ASSERT_FALSE(s.findPropertyIndex("uFragColor[0]").has_value()) << "shouldn't expose 'raw' name";
    ASSERT_TRUE(s.findPropertyIndex("uFragColor").has_value()) << "should work, because the backend should normalize array-like uniforms to the original name (not uFragColor[0])";
}

TEST_F(Renderer, ShaderGetPropertyTypeReturnsExpectedType)
{
    Shader s{c_VertexShaderSrc, c_FragmentShaderSrc};

    for (size_t i = 0; i < c_ExpectedPropertyNames.size(); ++i)
    {
        static_assert(c_ExpectedPropertyNames.size() == c_ExpectedPropertyTypes.size());
        auto const& propName = c_ExpectedPropertyNames[i];
        ShaderPropertyType expectedType = c_ExpectedPropertyTypes[i];

        std::optional<size_t> idx = s.findPropertyIndex(std::string{propName});
        ASSERT_TRUE(idx);
        ASSERT_EQ(s.getPropertyType(*idx), expectedType);
    }
}

TEST_F(Renderer, ShaderGetPropertyForCubemapReturnsExpectedType)
{
    Shader shader{c_CubemapVertexShader, c_CubemapFragmentShader};
    std::optional<size_t> index = shader.findPropertyIndex("skybox");

    ASSERT_TRUE(index.has_value());
    ASSERT_EQ(shader.getPropertyType(*index), ShaderPropertyType::SamplerCube);
}

TEST_F(Renderer, MaterialCanBeConstructed)
{
    GenerateMaterial();  // should compile and run fine
}

TEST_F(Renderer, MaterialCanBeCopyConstructed)
{
    Material material = GenerateMaterial();
    [[maybe_unused]] Material copied{material};
}

TEST_F(Renderer, MaterialCanBeMoveConstructed)
{
    Material material = GenerateMaterial();
    [[maybe_unused]] Material moved{std::move(material)};
}

TEST_F(Renderer, MaterialCanBeCopyAssigned)
{
    Material m1 = GenerateMaterial();
    Material m2 = GenerateMaterial();

    m1 = m2;
}

TEST_F(Renderer, MaterialCanBeMoveAssigned)
{
    Material m1 = GenerateMaterial();
    Material m2 = GenerateMaterial();

    m1 = std::move(m2);
}

TEST_F(Renderer, MaterialThatIsCopyConstructedEqualsSourceMaterial)
{
    Material material = GenerateMaterial();
    Material copy{material}; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(material, copy);
}

TEST_F(Renderer, MaterialThatIsCopyAssignedEqualsSourceMaterial)
{
    Material m1 = GenerateMaterial();
    Material m2 = GenerateMaterial();

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST_F(Renderer, MaterialGetShaderReturnsSuppliedShader)
{
    Shader shader{c_VertexShaderSrc, c_FragmentShaderSrc};
    Material material{shader};

    ASSERT_EQ(material.getShader(), shader);
}

TEST_F(Renderer, MaterialGetColorOnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getColor("someKey"));
}

TEST_F(Renderer, MaterialCanCallSetColorOnNewMaterial)
{
    Material mat = GenerateMaterial();
    mat.setColor("someKey", Color::red());
}

TEST_F(Renderer, MaterialCallingGetColorOnMaterialAfterSetColorReturnsTheColor)
{
    Material mat = GenerateMaterial();
    mat.setColor("someKey", Color::red());

    ASSERT_EQ(mat.getColor("someKey"), Color::red());
}

TEST_F(Renderer, MaterialGetColorArrayReturnsEmptyOnNewMaterial)
{
    Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getColorArray("someKey"));
}

TEST_F(Renderer, MaterialCanCallSetColorArrayOnNewMaterial)
{
    Material mat = GenerateMaterial();
    auto const colors = std::to_array({Color::black(), Color::blue()});

    mat.setColorArray("someKey", colors);
}

TEST_F(Renderer, MaterialCallingGetColorArrayOnMaterialAfterSettingThemReturnsTheSameColors)
{
    Material mat = GenerateMaterial();
    auto const colors = std::to_array({Color::red(), Color::green(), Color::blue()});
    CStringView const key = "someKey";

    mat.setColorArray(key, colors);

    std::optional<std::span<Color const>> rv = mat.getColorArray(key);

    ASSERT_TRUE(rv);
    ASSERT_EQ(std::size(*rv), std::size(colors));
    ASSERT_TRUE(std::equal(std::begin(colors), std::end(colors), std::begin(*rv)));
}
TEST_F(Renderer, MaterialGetFloatOnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getFloat("someKey"));
}

TEST_F(Renderer, MaterialGetFloatArrayOnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getFloatArray("someKey"));
}

TEST_F(Renderer, MaterialGetVec2OnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec2("someKey"));
}

TEST_F(Renderer, MaterialGetVec3OnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec3("someKey"));
}

TEST_F(Renderer, MaterialGetVec3ArrayOnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec3Array("someKey"));
}

TEST_F(Renderer, MaterialGetVec4OnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getVec4("someKey"));
}

TEST_F(Renderer, MaterialGetMat3OnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getMat3("someKey"));
}

TEST_F(Renderer, MaterialGetMat4OnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getMat4("someKey"));
}

TEST_F(Renderer, MaterialGetIntOnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getInt("someKey"));
}

TEST_F(Renderer, MaterialGetBoolOnNewMaterialReturnsEmptyOptional)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getBool("someKey"));
}

TEST_F(Renderer, MaterialSetFloatOnMaterialCausesGetFloatToReturnTheProvidedValue)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    float value = GenerateFloat();

    mat.setFloat(key, value);

    ASSERT_EQ(*mat.getFloat(key), value);
}

TEST_F(Renderer, MaterialSetFloatArrayOnMaterialCausesGetFloatArrayToReturnTheProvidedValues)
{
    Material mat = GenerateMaterial();
    std::string key = "someKey";
    std::array<float, 4> values = {GenerateFloat(), GenerateFloat(), GenerateFloat(), GenerateFloat()};

    ASSERT_FALSE(mat.getFloatArray(key));

    mat.setFloatArray(key, values);

    std::span<float const> rv = mat.getFloatArray(key).value();
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(Renderer, MaterialSetVec2OnMaterialCausesGetVec2ToReturnTheProvidedValue)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    Vec2 value = GenerateVec2();

    mat.setVec2(key, value);

    ASSERT_EQ(*mat.getVec2(key), value);
}

TEST_F(Renderer, MaterialSetVec2AndThenSetVec3CausesGetVec2ToReturnEmpty)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    Vec2 value = GenerateVec2();

    ASSERT_FALSE(mat.getVec2(key).has_value());

    mat.setVec2(key, value);

    ASSERT_TRUE(mat.getVec2(key).has_value());

    mat.setVec3(key, {});

    ASSERT_TRUE(mat.getVec3(key));
    ASSERT_FALSE(mat.getVec2(key));
}

TEST_F(Renderer, MaterialSetVec2CausesMaterialToCompareNotEqualToCopy)
{
    Material mat = GenerateMaterial();
    Material copy{mat};

    mat.setVec2("someKey", GenerateVec2());

    ASSERT_NE(mat, copy);
}

TEST_F(Renderer, MaterialSetVec3OnMaterialCausesGetVec3ToReturnTheProvidedValue)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    Vec3 value = GenerateVec3();

    mat.setVec3(key, value);

    ASSERT_EQ(*mat.getVec3(key), value);
}

TEST_F(Renderer, MaterialSetVec3ArrayOnMaterialCausesGetVec3ArrayToReutrnTheProvidedValues)
{
    Material mat = GenerateMaterial();
    std::string key = "someKey";
    std::array<Vec3, 4> values = {GenerateVec3(), GenerateVec3(), GenerateVec3(), GenerateVec3()};

    ASSERT_FALSE(mat.getVec3Array(key));

    mat.setVec3Array(key, values);

    std::span<Vec3 const> rv = mat.getVec3Array(key).value();
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(Renderer, MaterialSetVec4OnMaterialCausesGetVec4ToReturnTheProvidedValue)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    Vec4 value = GenerateVec4();

    mat.setVec4(key, value);

    ASSERT_EQ(*mat.getVec4(key), value);
}

TEST_F(Renderer, MaterialSetMat3OnMaterialCausesGetMat3ToReturnTheProvidedValue)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    Mat3 value = GenerateMat3x3();

    mat.setMat3(key, value);

    ASSERT_EQ(*mat.getMat3(key), value);
}

TEST_F(Renderer, MaterialSetMat4OnMaterialCausesGetMat4ToReturnTheProvidedValue)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    Mat4 value = GenerateMat4x4();

    mat.setMat4(key, value);

    ASSERT_EQ(*mat.getMat4(key), value);
}

TEST_F(Renderer, MaterialGetMat4ArrayInitiallyReturnsNothing)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getMat4Array("someKey").has_value());
}

TEST_F(Renderer, MaterialSetMat4ArrayCausesGetMat4ArrayToReturnSameSequenceOfValues)
{
    auto const mat4Array = std::to_array<Mat4>(
    {
        GenerateMat4x4(),
        GenerateMat4x4(),
        GenerateMat4x4(),
        GenerateMat4x4()
    });

    Material mat = GenerateMaterial();
    mat.setMat4Array("someKey", mat4Array);

    std::optional<std::span<Mat4 const>> rv = mat.getMat4Array("someKey");
    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(mat4Array.size(), rv->size());
    ASSERT_TRUE(std::equal(mat4Array.begin(), mat4Array.end(), rv->begin()));
}

TEST_F(Renderer, MaterialSetIntOnMaterialCausesGetIntToReturnTheProvidedValue)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    int value = GenerateInt();

    mat.setInt(key, value);

    ASSERT_EQ(*mat.getInt(key), value);
}

TEST_F(Renderer, MaterialSetBoolOnMaterialCausesGetBoolToReturnTheProvidedValue)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    bool value = GenerateBool();

    mat.setBool(key, value);

    ASSERT_EQ(*mat.getBool(key), value);
}

TEST_F(Renderer, MaterialSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    Texture2D t = GenerateTexture();

    ASSERT_FALSE(mat.getTexture(key));

    mat.setTexture(key, t);

    ASSERT_TRUE(mat.getTexture(key));
}

TEST_F(Renderer, MaterialClearTextureOnMaterialCausesGetTextureToReturnNothing)
{
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    Texture2D t = GenerateTexture();

    ASSERT_FALSE(mat.getTexture(key));

    mat.setTexture(key, t);

    ASSERT_TRUE(mat.getTexture(key));

    mat.clearTexture(key);

    ASSERT_FALSE(mat.getTexture(key));
}

TEST_F(Renderer, MaterialSetRenderTextureCausesGetRenderTextureToReturnTheTexture)
{
    Material mat = GenerateMaterial();
    RenderTexture renderTex = GenerateRenderTexture();
    std::string key = "someKey";

    ASSERT_FALSE(mat.getRenderTexture(key));

    mat.setRenderTexture(key, renderTex);

    ASSERT_EQ(*mat.getRenderTexture(key), renderTex);
}

TEST_F(Renderer, MaterialSetRenderTextureFollowedByClearRenderTextureClearsTheRenderTexture)
{
    Material mat = GenerateMaterial();
    RenderTexture renderTex = GenerateRenderTexture();
    std::string key = "someKey";

    ASSERT_FALSE(mat.getRenderTexture(key));

    mat.setRenderTexture(key, renderTex);

    ASSERT_EQ(*mat.getRenderTexture(key), renderTex);

    mat.clearRenderTexture(key);

    ASSERT_FALSE(mat.getRenderTexture(key));
}

TEST_F(Renderer, MaterialGetCubemapInitiallyReturnsNothing)
{
    Material const mat = GenerateMaterial();

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());
}

TEST_F(Renderer, MaterialGetCubemapReturnsSomethingAfterSettingCubemap)
{
    Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());

    Cubemap const cubemap{1, TextureFormat::RGBA32};

    mat.setCubemap("cubemap", cubemap);

    ASSERT_TRUE(mat.getCubemap("cubemap").has_value());
}

TEST_F(Renderer, MaterialGetCubemapReturnsTheCubemapThatWasLastSet)
{
    Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());

    Cubemap const firstCubemap{1, TextureFormat::RGBA32};
    Cubemap const secondCubemap{2, TextureFormat::RGBA32};  // different

    mat.setCubemap("cubemap", firstCubemap);
    ASSERT_EQ(mat.getCubemap("cubemap"), firstCubemap);

    mat.setCubemap("cubemap", secondCubemap);
    ASSERT_EQ(mat.getCubemap("cubemap"), secondCubemap);
}

TEST_F(Renderer, MaterialClearCubemapClearsTheCubemap)
{
    Material mat = GenerateMaterial();

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());

    Cubemap const cubemap{1, TextureFormat::RGBA32};

    mat.setCubemap("cubemap", cubemap);

    ASSERT_TRUE(mat.getCubemap("cubemap").has_value());

    mat.clearCubemap("cubemap");

    ASSERT_FALSE(mat.getCubemap("cubemap").has_value());
}

TEST_F(Renderer, MaterialGetTransparentIsInitiallyFalse)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getTransparent());
}

TEST_F(Renderer, MaterialSetTransparentBehavesAsExpected)
{
    Material mat = GenerateMaterial();
    mat.setTransparent(true);
    ASSERT_TRUE(mat.getTransparent());
    mat.setTransparent(false);
    ASSERT_FALSE(mat.getTransparent());
    mat.setTransparent(true);
    ASSERT_TRUE(mat.getTransparent());
}

TEST_F(Renderer, MaterialGetDepthTestedIsInitiallyTrue)
{
    Material mat = GenerateMaterial();
    ASSERT_TRUE(mat.getDepthTested());
}

TEST_F(Renderer, MaterialSetDepthTestedBehavesAsExpected)
{
    Material mat = GenerateMaterial();
    mat.setDepthTested(false);
    ASSERT_FALSE(mat.getDepthTested());
    mat.setDepthTested(true);
    ASSERT_TRUE(mat.getDepthTested());
    mat.setDepthTested(false);
    ASSERT_FALSE(mat.getDepthTested());
}

TEST_F(Renderer, MaterialGetDepthFunctionIsInitiallyDefault)
{
    Material mat = GenerateMaterial();
    ASSERT_EQ(mat.getDepthFunction(), DepthFunction::Default);
}

TEST_F(Renderer, MaterialSetDepthFunctionBehavesAsExpected)
{
    Material mat = GenerateMaterial();

    ASSERT_EQ(mat.getDepthFunction(), DepthFunction::Default);

    static_assert(DepthFunction::Default != DepthFunction::LessOrEqual);

    mat.setDepthFunction(DepthFunction::LessOrEqual);

    ASSERT_EQ(mat.getDepthFunction(), DepthFunction::LessOrEqual);
}

TEST_F(Renderer, MaterialGetWireframeModeIsInitiallyFalse)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getWireframeMode());
}

TEST_F(Renderer, MaterialSetWireframeModeBehavesAsExpected)
{
    Material mat = GenerateMaterial();
    mat.setWireframeMode(false);
    ASSERT_FALSE(mat.getWireframeMode());
    mat.setWireframeMode(true);
    ASSERT_TRUE(mat.getWireframeMode());
    mat.setWireframeMode(false);
    ASSERT_FALSE(mat.getWireframeMode());
}

TEST_F(Renderer, MaterialSetWireframeModeCausesMaterialCopiesToReturnNonEqual)
{
    Material mat = GenerateMaterial();
    ASSERT_FALSE(mat.getWireframeMode());
    Material copy{mat};
    ASSERT_EQ(mat, copy);
    copy.setWireframeMode(true);
    ASSERT_NE(mat, copy);
}

TEST_F(Renderer, MaterialGetCullModeIsInitiallyDefault)
{
    Material mat = GenerateMaterial();
    ASSERT_EQ(mat.getCullMode(), CullMode::Default);
}

TEST_F(Renderer, MaterialSetCullModeBehavesAsExpected)
{
    Material mat = GenerateMaterial();

    constexpr CullMode newCullMode = CullMode::Front;

    ASSERT_NE(mat.getCullMode(), newCullMode);
    mat.setCullMode(newCullMode);
    ASSERT_EQ(mat.getCullMode(), newCullMode);
}

TEST_F(Renderer, MaterialSetCullModeCausesMaterialCopiesToBeNonEqual)
{
    constexpr CullMode newCullMode = CullMode::Front;

    Material mat = GenerateMaterial();
    Material copy{mat};

    ASSERT_EQ(mat, copy);
    ASSERT_NE(mat.getCullMode(), newCullMode);
    mat.setCullMode(newCullMode);
    ASSERT_NE(mat, copy);
}

TEST_F(Renderer, MaterialCanCompareEquals)
{
    Material mat = GenerateMaterial();
    Material copy{mat};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(mat, copy);
}

TEST_F(Renderer, MaterialCanCompareNotEquals)
{
    Material m1 = GenerateMaterial();
    Material m2 = GenerateMaterial();

    ASSERT_NE(m1, m2);
}

TEST_F(Renderer, MaterialCanPrintToStringStream)
{
    Material m1 = GenerateMaterial();

    std::stringstream ss;
    ss << m1;
}

TEST_F(Renderer, MaterialOutputStringContainsUsefulInformation)
{
    Material m1 = GenerateMaterial();
    std::stringstream ss;

    ss << m1;

    std::string str{ss.str()};

    ASSERT_TRUE(ContainsCaseInsensitive(str, "Material"));

    // TODO: should print more useful info, such as number of props etc.
}

TEST_F(Renderer, MaterialSetFloatAndThenSetVec3CausesGetFloatToReturnEmpty)
{
    // compound test: when the caller sets a Vec3 then calling getInt with the same key should return empty
    Material mat = GenerateMaterial();

    std::string key = "someKey";
    float floatValue = GenerateFloat();
    Vec3 vecValue = GenerateVec3();

    mat.setFloat(key, floatValue);

    ASSERT_TRUE(mat.getFloat(key));

    mat.setVec3(key, vecValue);

    ASSERT_TRUE(mat.getVec3(key));
    ASSERT_FALSE(mat.getFloat(key));
}

TEST_F(Renderer, MaterialPropertyBlockCanDefaultConstruct)
{
    MaterialPropertyBlock mpb;
}

TEST_F(Renderer, MaterialPropertyBlockCanCopyConstruct)
{
    MaterialPropertyBlock mpb;
    MaterialPropertyBlock{mpb};
}

TEST_F(Renderer, MaterialPropertyBlockCanMoveConstruct)
{
    MaterialPropertyBlock mpb;
    MaterialPropertyBlock copy{std::move(mpb)};
}

TEST_F(Renderer, MaterialPropertyBlockCanCopyAssign)
{
    MaterialPropertyBlock m1;
    MaterialPropertyBlock m2;

    m1 = m2;
}

TEST_F(Renderer, MaterialPropertyBlockCanMoveAssign)
{
    MaterialPropertyBlock m1;
    MaterialPropertyBlock m2;

    m1 = std::move(m2);
}

TEST_F(Renderer, MaterialPropertyBlock)
{
    MaterialPropertyBlock mpb;

    ASSERT_TRUE(mpb.isEmpty());
}

TEST_F(Renderer, MaterialPropertyBlockCanClearDefaultConstructed)
{
    MaterialPropertyBlock mpb;
    mpb.clear();

    ASSERT_TRUE(mpb.isEmpty());
}

TEST_F(Renderer, MaterialPropertyBlockClearClearsProperties)
{
    MaterialPropertyBlock mpb;

    mpb.setFloat("someKey", GenerateFloat());

    ASSERT_FALSE(mpb.isEmpty());

    mpb.clear();

    ASSERT_TRUE(mpb.isEmpty());
}

TEST_F(Renderer, MaterialPropertyBlockGetColorOnNewMPBlReturnsEmptyOptional)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getColor("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockCanCallSetColorOnNewMaterial)
{
    MaterialPropertyBlock mpb;
    mpb.setColor("someKey", Color::red());
}

TEST_F(Renderer, MaterialPropertyBlockCallingGetColorOnMPBAfterSetColorReturnsTheColor)
{
    MaterialPropertyBlock mpb;
    mpb.setColor("someKey", Color::red());

    ASSERT_EQ(mpb.getColor("someKey"), Color::red());
}

TEST_F(Renderer, MaterialPropertyBlockGetFloatReturnsEmptyOnDefaultConstructedInstance)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getFloat("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetVec3ReturnsEmptyOnDefaultConstructedInstance)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getVec3("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetVec4ReturnsEmptyOnDefaultConstructedInstance)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getVec4("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetMat3ReturnsEmptyOnDefaultConstructedInstance)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getMat3("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetMat4ReturnsEmptyOnDefaultConstructedInstance)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getMat4("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetIntReturnsEmptyOnDefaultConstructedInstance)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getInt("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockGetBoolReturnsEmptyOnDefaultConstructedInstance)
{
    MaterialPropertyBlock mpb;
    ASSERT_FALSE(mpb.getBool("someKey"));
}

TEST_F(Renderer, MaterialPropertyBlockSetFloatCausesGetterToReturnSetValue)
{
    MaterialPropertyBlock mpb;
    std::string key = "someKey";
    float value = GenerateFloat();

    ASSERT_FALSE(mpb.getFloat(key));

    mpb.setFloat(key, value);
    ASSERT_TRUE(mpb.getFloat(key));
    ASSERT_EQ(mpb.getFloat(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetVec3CausesGetterToReturnSetValue)
{
    MaterialPropertyBlock mpb;
    std::string key = "someKey";
    Vec3 value = GenerateVec3();

    ASSERT_FALSE(mpb.getVec3(key));

    mpb.setVec3(key, value);
    ASSERT_TRUE(mpb.getVec3(key));
    ASSERT_EQ(mpb.getVec3(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetVec4CausesGetterToReturnSetValue)
{
    MaterialPropertyBlock mpb;
    std::string key = "someKey";
    Vec4 value = GenerateVec4();

    ASSERT_FALSE(mpb.getVec4(key));

    mpb.setVec4(key, value);
    ASSERT_TRUE(mpb.getVec4(key));
    ASSERT_EQ(mpb.getVec4(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetMat3CausesGetterToReturnSetValue)
{
    MaterialPropertyBlock mpb;
    std::string key = "someKey";
    Mat3 value = GenerateMat3x3();

    ASSERT_FALSE(mpb.getVec4(key));

    mpb.setMat3(key, value);
    ASSERT_TRUE(mpb.getMat3(key));
    ASSERT_EQ(mpb.getMat3(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetIntCausesGetterToReturnSetValue)
{
    MaterialPropertyBlock mpb;
    std::string key = "someKey";
    int value = GenerateInt();

    ASSERT_FALSE(mpb.getInt(key));

    mpb.setInt(key, value);
    ASSERT_TRUE(mpb.getInt(key));
    ASSERT_EQ(mpb.getInt(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetBoolCausesGetterToReturnSetValue)
{
    MaterialPropertyBlock mpb;
    std::string key = "someKey";
    bool value = GenerateBool();

    ASSERT_FALSE(mpb.getBool(key));

    mpb.setBool(key, value);
    ASSERT_TRUE(mpb.getBool(key));
    ASSERT_EQ(mpb.getBool(key), value);
}

TEST_F(Renderer, MaterialPropertyBlockSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    MaterialPropertyBlock mpb;

    std::string key = "someKey";
    Texture2D t = GenerateTexture();

    ASSERT_FALSE(mpb.getTexture(key));

    mpb.setTexture(key, t);

    ASSERT_TRUE(mpb.getTexture(key));
}

TEST_F(Renderer, MaterialPropertyBlockCanCompareEquals)
{
    MaterialPropertyBlock m1;
    MaterialPropertyBlock m2;

    ASSERT_TRUE(m1 == m2);
}

TEST_F(Renderer, MaterialPropertyBlockCopyConstructionComparesEqual)
{
    MaterialPropertyBlock m;
    MaterialPropertyBlock copy{m};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(m, copy);
}

TEST_F(Renderer, MaterialPropertyBlockCopyAssignmentComparesEqual)
{
    MaterialPropertyBlock m1;
    MaterialPropertyBlock m2;

    m1.setFloat("someKey", GenerateFloat());

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST_F(Renderer, MaterialPropertyBlockDifferentMaterialBlocksCompareNotEqual)
{
    MaterialPropertyBlock m1;
    MaterialPropertyBlock m2;

    m1.setFloat("someKey", GenerateFloat());

    ASSERT_NE(m1, m2);
}

TEST_F(Renderer, MaterialPropertyBlockCanPrintToOutputStream)
{
    MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;  // just ensure this compiles and runs
}

TEST_F(Renderer, MaterialPropertyBlockPrintingToOutputStreamMentionsMaterialPropertyBlock)
{
    MaterialPropertyBlock m1;
    std::stringstream ss;

    ss << m1;

    ASSERT_TRUE(Contains(ss.str(), "MaterialPropertyBlock"));
}

TEST_F(Renderer, TextureConstructorThrowsIfGivenZeroOrNegativeSizedDimensions)
{
    ASSERT_ANY_THROW({ Texture2D(Vec2i(0, 0)); });   // x and y are zero
    ASSERT_ANY_THROW({ Texture2D(Vec2i(0, 1)); });   // x is zero
    ASSERT_ANY_THROW({ Texture2D(Vec2i(1, 0)); });   // y is zero

    ASSERT_ANY_THROW({ Texture2D(Vec2i(-1, -1)); }); // x any y are negative
    ASSERT_ANY_THROW({ Texture2D(Vec2i(-1, 1)); });  // x is negative
    ASSERT_ANY_THROW({ Texture2D(Vec2i(1, -1)); });  // y is negative
}

TEST_F(Renderer, TextureDefaultConstructorCreatesRGBATextureWithExpectedColorSpaceEtc)
{
    Texture2D t{Vec2i(1, 1)};

    ASSERT_EQ(t.getDimensions(), Vec2i(1, 1));
    ASSERT_EQ(t.getTextureFormat(), TextureFormat::RGBA32);
    ASSERT_EQ(t.getColorSpace(), ColorSpace::sRGB);
    ASSERT_EQ(t.getWrapMode(), TextureWrapMode::Repeat);
    ASSERT_EQ(t.getFilterMode(), TextureFilterMode::Linear);
}

TEST_F(Renderer, TextureCanSetPixels32OnDefaultConstructedTexture)
{
    Vec2i const dimensions = {1, 1};
    std::vector<Color32> const pixels(static_cast<size_t>(dimensions.x * dimensions.y));

    Texture2D t{dimensions};
    t.setPixels32(pixels);

    ASSERT_EQ(t.getDimensions(), dimensions);
    ASSERT_EQ(t.getPixels32(), pixels);
}

TEST_F(Renderer, TextureSetPixelsThrowsIfNumberOfPixelsDoesNotMatchDimensions)
{
    Vec2i const dimensions = {1, 1};
    std::vector<Color> const incorrectPixels(dimensions.x * dimensions.y + 1);

    Texture2D t{dimensions};

    ASSERT_ANY_THROW({ t.setPixels(incorrectPixels); });
}

TEST_F(Renderer, TextureSetPixels32ThrowsIfNumberOfPixelsDoesNotMatchDimensions)
{
    Vec2i const dimensions = {1, 1};
    std::vector<Color32> const incorrectPixels(dimensions.x * dimensions.y + 1);

    Texture2D t{dimensions};
    ASSERT_ANY_THROW({ t.setPixels32(incorrectPixels); });
}

TEST_F(Renderer, TextureSetPixelDataThrowsIfNumberOfPixelBytesDoesNotMatchDimensions)
{
    Vec2i const dimensions = {1, 1};
    std::vector<Color32> const incorrectPixels(dimensions.x * dimensions.y + 1);

    Texture2D t{dimensions};

    ASSERT_EQ(t.getTextureFormat(), TextureFormat::RGBA32);  // sanity check
    ASSERT_ANY_THROW({ t.setPixelData(ViewObjectRepresentations<uint8_t>(incorrectPixels)); });
}

TEST_F(Renderer, TextureSetPixelDataDoesNotThrowWhenGivenValidNumberOfPixelBytes)
{
    Vec2i const dimensions = {1, 1};
    std::vector<Color32> const pixels(static_cast<size_t>(dimensions.x * dimensions.y));

    Texture2D t{dimensions};

    ASSERT_EQ(t.getTextureFormat(), TextureFormat::RGBA32);  // sanity check

    t.setPixelData(ViewObjectRepresentations<uint8_t>(pixels));
}

TEST_F(Renderer, TextureSetPixelDataWorksFineFor8BitSingleChannelData)
{
    Vec2i const dimensions = {1, 1};
    std::vector<uint8_t> const singleChannelPixels(static_cast<size_t>(dimensions.x * dimensions.y));

    Texture2D t{dimensions, TextureFormat::R8};
    t.setPixelData(singleChannelPixels);  // shouldn't throw
}

TEST_F(Renderer, TextureSetPixelDataWith8BitSingleChannelDataFollowedByGetPixelsBlanksOutGreenAndRed)
{
    uint8_t const color{0x88};
    float const colorFloat = static_cast<float>(color) / 255.0f;
    Vec2i const dimensions = {1, 1};
    std::vector<uint8_t> const singleChannelPixels(static_cast<size_t>(dimensions.x * dimensions.y), color);

    Texture2D t{dimensions, TextureFormat::R8};
    t.setPixelData(singleChannelPixels);

    for (Color const& c : t.getPixels())
    {
        ASSERT_EQ(c, Color(colorFloat, 0.0f, 0.0f, 1.0f));
    }
}

TEST_F(Renderer, TextureSetPixelDataWith8BitSingleChannelDataFollowedByGetPixels32BlanksOutGreenAndRed)
{
    uint8_t const color{0x88};
    Vec2i const dimensions = {1, 1};
    std::vector<uint8_t> const singleChannelPixels(static_cast<size_t>(dimensions.x * dimensions.y), color);

    Texture2D t{dimensions, TextureFormat::R8};
    t.setPixelData(singleChannelPixels);

    for (Color32 const& c : t.getPixels32())
    {
        Color32 expected{color, 0x00, 0x00, 0xff};
        ASSERT_EQ(c, expected);
    }
}

TEST_F(Renderer, TextureSetPixelDataWith32BitFloatingPointValuesFollowedByGetPixelDataReturnsSameSpan)
{
    Vec4 const color = GenerateVec4();
    Vec2i const dimensions = {1, 1};
    std::vector<Vec4> const rgbaFloatPixels(static_cast<size_t>(dimensions.x * dimensions.y), color);

    Texture2D t(dimensions, TextureFormat::RGBAFloat);
    t.setPixelData(ViewObjectRepresentations<uint8_t>(rgbaFloatPixels));

    ASSERT_TRUE(ContainersEqual(t.getPixelData(), ViewObjectRepresentations<uint8_t>(rgbaFloatPixels)));
}

TEST_F(Renderer, TextureSetPixelDataWith32BitFloatingPointValuesFollowedByGetPixelsReturnsSameValues)
{
    Color const hdrColor = {1.2f, 1.4f, 1.3f, 1.0f};
    Vec2i const dimensions = {1, 1};
    std::vector<Color> const rgbaFloatPixels(static_cast<size_t>(dimensions.x * dimensions.y), hdrColor);

    Texture2D t(dimensions, TextureFormat::RGBAFloat);
    t.setPixelData(ViewObjectRepresentations<uint8_t>(rgbaFloatPixels));

    ASSERT_EQ(t.getPixels(), rgbaFloatPixels);  // because the texture holds 32-bit floats
}

TEST_F(Renderer, TextureSetPixelsOnAn8BitTextureLDRClampsTheColorValues)
{
    Color const hdrColor = {1.2f, 1.4f, 1.3f, 1.0f};
    Vec2i const dimensions = {1, 1};
    std::vector<Color> const hdrPixels(static_cast<size_t>(dimensions.x * dimensions.y), hdrColor);

    Texture2D t(dimensions, TextureFormat::RGBA32);  // note: not HDR

    t.setPixels(hdrPixels);

    ASSERT_NE(t.getPixels(), hdrPixels);  // because the impl had to convert them
}

TEST_F(Renderer, TextureSetPixels32OnAn8BitTextureDoesntConvert)
{
    Color32 const color32 = {0x77, 0x63, 0x24, 0x76};
    Vec2i const dimensions = {1, 1};
    std::vector<Color32> const pixels32(static_cast<size_t>(dimensions.x * dimensions.y), color32);

    Texture2D t(dimensions, TextureFormat::RGBA32);  // note: matches pixel format

    t.setPixels32(pixels32);

    ASSERT_EQ(t.getPixels32(), pixels32);  // because no conversion was required
}

TEST_F(Renderer, TextureSetPixels32OnA32BitTextureDoesntDetectablyChangeValues)
{
    Color32 const color32 = {0x77, 0x63, 0x24, 0x76};
    Vec2i const dimensions = {1, 1};
    std::vector<Color32> const pixels32(static_cast<size_t>(dimensions.x * dimensions.y), color32);

    Texture2D t(dimensions, TextureFormat::RGBAFloat);  // note: higher precision than input

    t.setPixels32(pixels32);

    ASSERT_EQ(t.getPixels32(), pixels32);  // because, although conversion happened, it was _from_ a higher precision
}

TEST_F(Renderer, TextureCanCopyConstruct)
{
    Texture2D t = GenerateTexture();
    Texture2D{t};
}

TEST_F(Renderer, TextureCanMoveConstruct)
{
    Texture2D t = GenerateTexture();
    Texture2D copy{std::move(t)};
}

TEST_F(Renderer, TextureCanCopyAssign)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2 = GenerateTexture();

    t1 = t2;
}

TEST_F(Renderer, TextureCanMoveAssign)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2 = GenerateTexture();

    t1 = std::move(t2);
}

TEST_F(Renderer, TextureGetWidthReturnsSuppliedWidth)
{
    int width = 2;
    int height = 6;

    Texture2D t{{width, height}};

    ASSERT_EQ(t.getDimensions().x, width);
}

TEST_F(Renderer, TextureGetHeightReturnsSuppliedHeight)
{
    int width = 2;
    int height = 6;

    Texture2D t{{width, height}};

    ASSERT_EQ(t.getDimensions().y, height);
}

TEST_F(Renderer, TextureGetColorSpaceReturnsProvidedColorSpaceIfSRGB)
{
    Texture2D t{{1, 1}, TextureFormat::RGBA32, ColorSpace::sRGB};

    ASSERT_EQ(t.getColorSpace(), ColorSpace::sRGB);
}

TEST_F(Renderer, TextureGetColorSpaceReturnsProvidedColorSpaceIfLinear)
{
    Texture2D t{{1, 1}, TextureFormat::RGBA32, ColorSpace::Linear};

    ASSERT_EQ(t.getColorSpace(), ColorSpace::Linear);
}

TEST_F(Renderer, TextureGetWrapModeReturnsRepeatedByDefault)
{
    Texture2D t = GenerateTexture();

    ASSERT_EQ(t.getWrapMode(), TextureWrapMode::Repeat);
}

TEST_F(Renderer, TextureSetWrapModeMakesSubsequentGetWrapModeReturnNewWrapMode)
{
    Texture2D t = GenerateTexture();

    TextureWrapMode wm = TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapMode(), wm);

    t.setWrapMode(wm);

    ASSERT_EQ(t.getWrapMode(), wm);
}

TEST_F(Renderer, TextureSetWrapModeCausesGetWrapModeUToAlsoReturnNewWrapMode)
{
    Texture2D t = GenerateTexture();

    TextureWrapMode wm = TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapMode(), wm);
    ASSERT_NE(t.getWrapModeU(), wm);

    t.setWrapMode(wm);

    ASSERT_EQ(t.getWrapModeU(), wm);
}

TEST_F(Renderer, TextureSetWrapModeUCausesGetWrapModeUToReturnValue)
{
    Texture2D t = GenerateTexture();

    TextureWrapMode wm = TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeU(), wm);

    t.setWrapModeU(wm);

    ASSERT_EQ(t.getWrapModeU(), wm);
}

TEST_F(Renderer, TextureSetWrapModeVCausesGetWrapModeVToReturnValue)
{
    Texture2D t = GenerateTexture();

    TextureWrapMode wm = TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeV(), wm);

    t.setWrapModeV(wm);

    ASSERT_EQ(t.getWrapModeV(), wm);
}

TEST_F(Renderer, TextureSetWrapModeWCausesGetWrapModeWToReturnValue)
{
    Texture2D t = GenerateTexture();

    TextureWrapMode wm = TextureWrapMode::Mirror;

    ASSERT_NE(t.getWrapModeW(), wm);

    t.setWrapModeW(wm);

    ASSERT_EQ(t.getWrapModeW(), wm);
}

TEST_F(Renderer, TextureSetFilterModeCausesGetFilterModeToReturnValue)
{
    Texture2D t = GenerateTexture();

    TextureFilterMode tfm = TextureFilterMode::Nearest;

    ASSERT_NE(t.getFilterMode(), tfm);

    t.setFilterMode(tfm);

    ASSERT_EQ(t.getFilterMode(), tfm);
}

TEST_F(Renderer, TextureSetFilterModeMipmapReturnsMipmapOnGetFilterMode)
{
    Texture2D t = GenerateTexture();

    TextureFilterMode tfm = TextureFilterMode::Mipmap;

    ASSERT_NE(t.getFilterMode(), tfm);

    t.setFilterMode(tfm);

    ASSERT_EQ(t.getFilterMode(), tfm);
}

TEST_F(Renderer, TextureCanBeComparedForEquality)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2 = GenerateTexture();

    (void)(t1 == t2);  // just ensure it compiles + runs
}

TEST_F(Renderer, TextureCopyConstructingComparesEqual)
{
    Texture2D t = GenerateTexture();
    Texture2D tcopy{t};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(t, tcopy);
}

TEST_F(Renderer, TextureCopyAssignmentMakesEqualityReturnTrue)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2 = GenerateTexture();

    t1 = t2;

    ASSERT_EQ(t1, t2);
}

TEST_F(Renderer, TextureCanBeComparedForNotEquals)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2 = GenerateTexture();

    (void)(t1 != t2);  // just ensure this expression compiles
}

TEST_F(Renderer, TextureChangingWrapModeMakesCopyUnequal)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2{t1};
    TextureWrapMode wm = TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapMode(), wm);

    t2.setWrapMode(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingWrapModeUMakesCopyUnequal)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2{t1};
    TextureWrapMode wm = TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeU(), wm);

    t2.setWrapModeU(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingWrapModeVMakesCopyUnequal)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2{t1};
    TextureWrapMode wm = TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeV(), wm);

    t2.setWrapModeV(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingWrapModeWMakesCopyUnequal)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2{t1};
    TextureWrapMode wm = TextureWrapMode::Clamp;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getWrapModeW(), wm);

    t2.setWrapModeW(wm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureChangingFilterModeMakesCopyUnequal)
{
    Texture2D t1 = GenerateTexture();
    Texture2D t2{t1};
    TextureFilterMode fm = TextureFilterMode::Nearest;

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t2.getFilterMode(), fm);

    t2.setFilterMode(fm);

    ASSERT_NE(t1, t2);
}

TEST_F(Renderer, TextureCanBeWrittenToOutputStream)
{
    Texture2D t = GenerateTexture();

    std::stringstream ss;
    ss << t;

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(Renderer, MeshTopologyAllCanBeWrittenToStream)
{
    for (size_t i = 0; i < NumOptions<MeshTopology>(); ++i)
    {
        auto const mt = static_cast<MeshTopology>(i);

        std::stringstream ss;

        ss << mt;

        ASSERT_FALSE(ss.str().empty());
    }
}

TEST_F(Renderer, LoadTexture2DFromImageResourceCanLoadImageFile)
{
    Texture2D const t = osc::LoadTexture2DFromImage(
        App::resource((std::filesystem::path{OSC_BUILD_RESOURCES_DIR} / "testoscar/awesomeface.png").string()),
        ColorSpace::sRGB
    );
    ASSERT_EQ(t.getDimensions(), Vec2i(512, 512));
}

TEST_F(Renderer, LoadTexture2DFromImageResourceThrowsIfResourceNotFound)
{
    ASSERT_ANY_THROW(
    {
        osc::LoadTexture2DFromImage(
            App::resource("textures/doesnt_exist.png"),
            ColorSpace::sRGB
        );
    });
}

TEST_F(Renderer, RenderTextureFormatCanBeIteratedOverAndStreamedToString)
{
    for (size_t i = 0; i < NumOptions<RenderTextureFormat>(); ++i)
    {
        std::stringstream ss;
        ss << static_cast<RenderTextureFormat>(i);  // shouldn't throw
    }
}

TEST_F(Renderer, DepthStencilFormatCanBeIteratedOverAndStreamedToString)
{
    for (size_t i = 0; i < NumOptions<DepthStencilFormat>(); ++i)
    {
        std::stringstream ss;
        ss << static_cast<DepthStencilFormat>(i);  // shouldn't throw
    }
}

TEST_F(Renderer, DrawMeshDoesNotThrowWithStandardArgs)
{
    Mesh const mesh;
    Transform const transform = Identity<Transform>();
    Material const material = GenerateMaterial();
    Camera camera;

    ASSERT_NO_THROW({ osc::Graphics::DrawMesh(mesh, transform, material, camera); });
}

TEST_F(Renderer, DrawMeshThrowsIfGivenOutOfBoundsSubMeshIndex)
{
    Mesh const mesh;
    Transform const transform = Identity<Transform>();
    Material const material = GenerateMaterial();
    Camera camera;

    ASSERT_ANY_THROW({ osc::Graphics::DrawMesh(mesh, transform, material, camera, std::nullopt, 0); });
}

TEST_F(Renderer, DrawMeshDoesNotThrowIfGivenInBoundsSubMesh)
{
    Mesh mesh;
    mesh.pushSubMeshDescriptor({0, 0, MeshTopology::Triangles});
    Transform const transform = Identity<Transform>();
    Material const material = GenerateMaterial();
    Camera camera;

    ASSERT_NO_THROW({ osc::Graphics::DrawMesh(mesh, transform, material, camera, std::nullopt, 0); });
}
