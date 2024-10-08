// multiple associated headers: this should be broken up into
// smaller pieces

#include <testoscar/TestingHelpers.h>
#include <testoscar/testoscarconfig.h>

#include <gtest/gtest.h>
#include <oscar/Formats/Image.h>
#include <oscar/Graphics/Materials/MeshDepthWritingMaterial.h>
#include <oscar/Graphics/Materials/MeshNormalVectorsMaterial.h>
#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/ColorRenderBufferFormat.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Cubemap.h>
#include <oscar/Graphics/CullMode.h>
#include <oscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/MeshTopology.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/ShaderPropertyType.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Maths/Mat3.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/EnumHelpers.h>
#include <oscar/Utils/StringHelpers.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>

namespace graphics = osc::graphics;
using namespace osc::testing;
using namespace osc;

namespace
{
    std::unique_ptr<App> g_renderer_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    class Renderer : public ::testing::Test {
    protected:
        static void SetUpTestSuite()
        {
            const AppMetadata metadata{TESTOSCAR_ORGNAME_STRING, TESTOSCAR_APPNAME_STRING};
            g_renderer_app = std::make_unique<App>(metadata);
        }

        static void TearDownTestSuite()
        {
            g_renderer_app.reset();
        }
    };

    constexpr CStringView c_vertex_shader_src = R"(
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

    constexpr CStringView c_fragment_shader_src = R"(
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

    constexpr CStringView c_vertex_shader_with_array = R"(
        #version 330 core

        layout (location = 0) in vec3 aPos;

        void main()
        {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    constexpr CStringView c_fragment_shader_with_array = R"(
        #version 330 core

        uniform vec4 uFragColor[3];

        out vec4 FragColor;

        void main()
        {
            FragColor = uFragColor[0];
        }
    )";

    // expected, based on the above shader code
    constexpr std::array<CStringView, 14> c_expected_property_names = std::to_array<CStringView>(
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

    constexpr std::array<ShaderPropertyType, 14> c_expected_property_types = std::to_array(
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
    static_assert(c_expected_property_names.size() == c_expected_property_types.size());

    constexpr CStringView c_geometry_shader_vert_src = R"(
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

    constexpr CStringView c_geometry_shader_geom_src = R"(
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

    constexpr CStringView c_geometry_shader_frag_src = R"(
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
    constexpr CStringView c_cubemap_vertex_shader = R"(
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

    constexpr CStringView c_cubemap_fragment_shader = R"(
        #version 330 core

        out vec4 FragColor;

        in vec3 TexCoords;

        uniform samplerCube skybox;

        void main()
        {
            FragColor = texture(skybox, TexCoords);
        }
    )";

    Texture2D generate_red_texture()
    {
        Texture2D rv{Vec2i{2, 2}};
        rv.set_pixels(std::vector<Color>(4, Color::red()));
        return rv;
    }

    Material generate_material()
    {
        const Shader shader{c_vertex_shader_src, c_fragment_shader_src};
        return Material{shader};
    }

    RenderTexture generate_render_texture()
    {
        return RenderTexture{{.dimensions = {2, 2}}};
    }
}

TEST_F(Renderer, ShaderCanBeConstructedFromVertexAndFragmentShaderSource)
{
    Shader s{c_vertex_shader_src, c_fragment_shader_src};
}

TEST_F(Renderer, ShaderCanBeConstructedFromVertexGeometryAndFragmentShaderSources)
{
    Shader s{c_geometry_shader_vert_src, c_geometry_shader_geom_src, c_geometry_shader_frag_src};
}

TEST_F(Renderer, ShaderCanBeCopyConstructed)
{
    Shader s{c_vertex_shader_src, c_fragment_shader_src};
    Shader{s};
}

TEST_F(Renderer, ShaderCanBeMoveConstructed)
{
    Shader s{c_vertex_shader_src, c_fragment_shader_src};
    Shader copy{std::move(s)};
}

TEST_F(Renderer, ShaderCanBeCopyAssigned)
{
    Shader s1{c_vertex_shader_src, c_fragment_shader_src};
    Shader s2{c_vertex_shader_src, c_fragment_shader_src};

    s1 = s2;
}

TEST_F(Renderer, ShaderCanBeMoveAssigned)
{
    Shader s1{c_vertex_shader_src, c_fragment_shader_src};
    Shader s2{c_vertex_shader_src, c_fragment_shader_src};

    s1 = std::move(s2);
}

TEST_F(Renderer, ShaderThatIsCopyConstructedEqualsSrcShader)
{
    Shader s{c_vertex_shader_src, c_fragment_shader_src};
    Shader copy{s}; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(s, copy);
}

TEST_F(Renderer, ShadersThatDifferCompareNotEqual)
{
    Shader s1{c_vertex_shader_src, c_fragment_shader_src};
    Shader s2{c_vertex_shader_src, c_fragment_shader_src};

    ASSERT_NE(s1, s2);
}

TEST_F(Renderer, ShaderCanBeWrittenToOutputStream)
{
    Shader s{c_vertex_shader_src, c_fragment_shader_src};

    std::stringstream ss;
    ss << s;  // shouldn't throw etc.

    ASSERT_FALSE(ss.str().empty());
}

TEST_F(Renderer, ShaderOutputStreamContainsExpectedInfo)
{
    // this test is flakey, but is just ensuring that the string printout has enough information
    // to help debugging etc.

    Shader s{c_vertex_shader_src, c_fragment_shader_src};

    std::stringstream ss;
    ss << s;
    std::string str{std::move(ss).str()};

    for (const auto& propName : c_expected_property_names) {
        ASSERT_TRUE(contains(str, std::string{propName}));
    }
}

TEST_F(Renderer, ShaderFindPropertyIndexCanFindAllExpectedProperties)
{
    Shader s{c_vertex_shader_src, c_fragment_shader_src};

    for (const auto& propName : c_expected_property_names) {
        ASSERT_TRUE(s.property_index(std::string{propName}));
    }
}
TEST_F(Renderer, ShaderHasExpectedNumberOfProperties)
{
    // (effectively, number of properties == number of uniforms)
    Shader s{c_vertex_shader_src, c_fragment_shader_src};
    ASSERT_EQ(s.num_properties(), c_expected_property_names.size());
}

TEST_F(Renderer, ShaderIteratingOverPropertyIndicesForNameReturnsValidPropertyName)
{
    Shader s{c_vertex_shader_src, c_fragment_shader_src};

    std::unordered_set<std::string> allPropNames;
    for (const auto& sv : c_expected_property_names) {
        allPropNames.insert(std::string{sv});
    }
    std::unordered_set<std::string> returnedPropNames;

    for (size_t i = 0, len = s.num_properties(); i < len; ++i) {
        returnedPropNames.insert(std::string{s.property_name(i)});
    }

    ASSERT_EQ(allPropNames, returnedPropNames);
}

TEST_F(Renderer, ShaderGetPropertyNameReturnsGivenPropertyName)
{
    Shader s{c_vertex_shader_src, c_fragment_shader_src};

    for (const auto& propName : c_expected_property_names) {
        std::optional<size_t> idx = s.property_index(propName);
        ASSERT_TRUE(idx);
        ASSERT_EQ(s.property_name(*idx), propName);
    }
}

TEST_F(Renderer, ShaderGetPropertyNameStillWorksIfTheUniformIsAnArray)
{
    Shader s{c_vertex_shader_with_array, c_fragment_shader_with_array};
    ASSERT_FALSE(s.property_index("uFragColor[0]").has_value()) << "shouldn't expose 'raw' name";
    ASSERT_TRUE(s.property_index("uFragColor").has_value()) << "should work, because the backend should normalize array-like uniforms to the original name (not uFragColor[0])";
}

TEST_F(Renderer, ShaderGetPropertyTypeReturnsExpectedType)
{
    Shader s{c_vertex_shader_src, c_fragment_shader_src};

    for (size_t i = 0; i < c_expected_property_names.size(); ++i) {
        static_assert(c_expected_property_names.size() == c_expected_property_types.size());
        const auto& propName = c_expected_property_names[i];
        ShaderPropertyType expectedType = c_expected_property_types[i];

        std::optional<size_t> idx = s.property_index(std::string{propName});
        ASSERT_TRUE(idx);
        ASSERT_EQ(s.property_type(*idx), expectedType);
    }
}

TEST_F(Renderer, ShaderGetPropertyForCubemapReturnsExpectedType)
{
    Shader shader{c_cubemap_vertex_shader, c_cubemap_fragment_shader};
    std::optional<size_t> index = shader.property_index("skybox");

    ASSERT_TRUE(index.has_value());
    ASSERT_EQ(shader.property_type(*index), ShaderPropertyType::SamplerCube);
}

TEST_F(Renderer, MaterialCanBeConstructed)
{
    generate_material();  // should compile and run fine
}

TEST_F(Renderer, MaterialCanBeCopyConstructed)
{
    Material material = generate_material();
    Material{material};
}

TEST_F(Renderer, MaterialCanBeMoveConstructed)
{
    Material material = generate_material();
    Material{std::move(material)};
}

TEST_F(Renderer, MaterialCanBeCopyAssigned)
{
    Material m1 = generate_material();
    Material m2 = generate_material();

    m1 = m2;
}

TEST_F(Renderer, MaterialCanBeMoveAssigned)
{
    Material m1 = generate_material();
    Material m2 = generate_material();

    m1 = std::move(m2);
}

TEST_F(Renderer, MaterialThatIsCopyConstructedEqualsSourceMaterial)
{
    Material material = generate_material();
    Material copy{material}; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(material, copy);
}

TEST_F(Renderer, MaterialThatIsCopyAssignedEqualsSourceMaterial)
{
    Material m1 = generate_material();
    Material m2 = generate_material();

    ASSERT_NE(m1, m2);

    m1 = m2;

    ASSERT_EQ(m1, m2);
}

TEST_F(Renderer, MaterialGetShaderReturnsSuppliedShader)
{
    Shader shader{c_vertex_shader_src, c_fragment_shader_src};
    Material material{shader};

    ASSERT_EQ(material.shader(), shader);
}

TEST_F(Renderer, MaterialGetColorOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();

    ASSERT_FALSE(mat.get<Color>("someKey"));
}

TEST_F(Renderer, MaterialCanCallSetColorOnNewMaterial)
{
    Material mat = generate_material();
    mat.set<Color>("someKey", Color::red());
}

TEST_F(Renderer, MaterialCallingGetColorOnMaterialAfterSetColorReturnsTheColor)
{
    Material mat = generate_material();
    mat.set<Color>("someKey", Color::red());

    ASSERT_EQ(mat.get<Color>("someKey"), Color::red());
}

TEST_F(Renderer, MaterialGetColorArrayReturnsEmptyOnNewMaterial)
{
    Material mat = generate_material();

    ASSERT_FALSE(mat.get_array<Color>("someKey"));
}

TEST_F(Renderer, MaterialCanCallSetColorArrayOnNewMaterial)
{
    Material mat = generate_material();
    const auto colors = std::to_array({Color::black(), Color::blue()});

    mat.set_array<Color>("someKey", colors);
}

TEST_F(Renderer, MaterialCallingGetColorArrayOnMaterialAfterSettingThemReturnsTheSameColors)
{
    Material mat = generate_material();
    const auto colors = std::to_array({Color::red(), Color::green(), Color::blue()});
    const CStringView key = "someKey";

    mat.set_array<Color>(key, colors);

    std::optional<std::span<const Color>> rv = mat.get_array<Color>(key);

    ASSERT_TRUE(rv);
    ASSERT_EQ(std::size(*rv), std::size(colors));
    ASSERT_TRUE(std::equal(std::begin(colors), std::end(colors), std::begin(*rv)));
}
TEST_F(Renderer, MaterialGetFloatOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<float>("someKey"));
}

TEST_F(Renderer, MaterialGetFloatArrayOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get_array<float>("someKey"));
}

TEST_F(Renderer, MaterialGetVec2OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Vec2>("someKey"));
}

TEST_F(Renderer, MaterialGetVec3OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Vec3>("someKey"));
}

TEST_F(Renderer, MaterialGetVec3ArrayOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get_array<Vec3>("someKey"));
}

TEST_F(Renderer, MaterialGetVec4OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Vec4>("someKey"));
}

TEST_F(Renderer, MaterialGetMat3OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Mat3>("someKey"));
}

TEST_F(Renderer, MaterialGetMat4OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Mat4>("someKey"));
}

TEST_F(Renderer, MaterialGetIntOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<int>("someKey"));
}

TEST_F(Renderer, MaterialGetBoolOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<bool>("someKey"));
}

TEST_F(Renderer, MaterialSetFloatOnMaterialCausesGetFloatToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    float value = generate<float>();

    mat.set<float>(key, value);

    ASSERT_EQ(*mat.get<float>(key), value);
}

TEST_F(Renderer, MaterialSetFloatArrayOnMaterialCausesGetFloatArrayToReturnTheProvidedValues)
{
    Material mat = generate_material();
    std::string key = "someKey";
    std::array<float, 4> values = {generate<float>(), generate<float>(), generate<float>(), generate<float>()};

    ASSERT_FALSE(mat.get_array<float>(key));

    mat.set_array<float>(key, values);

    std::span<const float> rv = mat.get_array<float>(key).value();
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(Renderer, MaterialSetVec2OnMaterialCausesGetVec2ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Vec2 value = generate<Vec2>();

    mat.set<Vec2>(key, value);

    ASSERT_EQ(*mat.get<Vec2>(key), value);
}

TEST_F(Renderer, MaterialSetVec2AndThenSetVec3CausesGetVec2ToReturnEmpty)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Vec2 value = generate<Vec2>();

    ASSERT_FALSE(mat.get<Vec2>(key).has_value());

    mat.set<Vec2>(key, value);

    ASSERT_TRUE(mat.get<Vec2>(key).has_value());

    mat.set<Vec3>(key, {});

    ASSERT_TRUE(mat.get<Vec3>(key));
    ASSERT_FALSE(mat.get<Vec2>(key));
}

TEST_F(Renderer, MaterialSetVec2CausesMaterialToCompareNotEqualToCopy)
{
    Material mat = generate_material();
    Material copy{mat};

    mat.set<Vec2>("someKey", generate<Vec2>());

    ASSERT_NE(mat, copy);
}

TEST_F(Renderer, MaterialSetVec3OnMaterialCausesGetVec3ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Vec3 value = generate<Vec3>();

    mat.set<Vec3>(key, value);

    ASSERT_EQ(*mat.get<Vec3>(key), value);
}

TEST_F(Renderer, MaterialSetVec3ArrayOnMaterialCausesGetVec3ArrayToReutrnTheProvidedValues)
{
    Material mat = generate_material();
    std::string key = "someKey";
    std::array<Vec3, 4> values = {generate<Vec3>(), generate<Vec3>(), generate<Vec3>(), generate<Vec3>()};

    ASSERT_FALSE(mat.get_array<Vec3>(key));

    mat.set_array<Vec3>(key, values);

    std::span<const Vec3> rv = mat.get_array<Vec3>(key).value();
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(Renderer, MaterialSetVec4OnMaterialCausesGetVec4ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Vec4 value = generate<Vec4>();

    mat.set<Vec4>(key, value);

    ASSERT_EQ(*mat.get<Vec4>(key), value);
}

TEST_F(Renderer, MaterialSetMat3OnMaterialCausesGetMat3ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Mat3 value = generate<Mat3>();

    mat.set<Mat3>(key, value);

    ASSERT_EQ(*mat.get<Mat3>(key), value);
}

TEST_F(Renderer, MaterialSetMat4OnMaterialCausesGetMat4ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Mat4 value = generate<Mat4>();

    mat.set<Mat4>(key, value);

    ASSERT_EQ(*mat.get<Mat4>(key), value);
}

TEST_F(Renderer, MaterialGetMat4ArrayInitiallyReturnsNothing)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get_array<Mat4>("someKey").has_value());
}

TEST_F(Renderer, MaterialSetMat4ArrayCausesGetMat4ArrayToReturnSameSequenceOfValues)
{
    const auto mat4Array = std::to_array<Mat4>({
        generate<Mat4>(),
        generate<Mat4>(),
        generate<Mat4>(),
        generate<Mat4>()
    });

    Material mat = generate_material();
    mat.set_array<Mat4>("someKey", mat4Array);

    std::optional<std::span<const Mat4>> rv = mat.get_array<Mat4>("someKey");
    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(mat4Array.size(), rv->size());
    ASSERT_TRUE(std::equal(mat4Array.begin(), mat4Array.end(), rv->begin()));
}

TEST_F(Renderer, MaterialSetIntOnMaterialCausesGetIntToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    int value = generate<int>();

    mat.set<int>(key, value);

    ASSERT_EQ(*mat.get<int>(key), value);
}

TEST_F(Renderer, MaterialSetBoolOnMaterialCausesGetBoolToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    bool value = generate<bool>();

    mat.set<bool>(key, value);

    ASSERT_EQ(*mat.get<bool>(key), value);
}

TEST_F(Renderer, MaterialSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Texture2D t = generate_red_texture();

    ASSERT_FALSE(mat.get<Texture2D>(key));

    mat.set(key, t);

    ASSERT_TRUE(mat.get<Texture2D>(key));
}

TEST_F(Renderer, MaterialUnsetTextureOnMaterialCausesGetTextureToReturnNothing)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Texture2D t = generate_red_texture();

    ASSERT_FALSE(mat.get<Texture2D>(key));

    mat.set(key, t);

    ASSERT_TRUE(mat.get<Texture2D>(key));

    mat.unset(key);

    ASSERT_FALSE(mat.get<Texture2D>(key));
}

TEST_F(Renderer, MaterialSetRenderTextureCausesGetRenderTextureToReturnTheTexture)
{
    Material mat = generate_material();
    RenderTexture renderTex = generate_render_texture();
    std::string key = "someKey";

    ASSERT_FALSE(mat.get<RenderTexture>(key));

    mat.set(key, renderTex);

    ASSERT_EQ(*mat.get<RenderTexture>(key), renderTex);
}

TEST_F(Renderer, MaterialSetRenderTextureFollowedByUnsetClearsTheRenderTexture)
{
    Material mat = generate_material();
    RenderTexture renderTex = generate_render_texture();
    std::string key = "someKey";

    ASSERT_FALSE(mat.get<RenderTexture>(key));

    mat.set(key, renderTex);

    ASSERT_EQ(*mat.get<RenderTexture>(key), renderTex);

    mat.unset(key);

    ASSERT_FALSE(mat.get<RenderTexture>(key));
}

TEST_F(Renderer, MaterialGetCubemapInitiallyReturnsNothing)
{
    const Material mat = generate_material();

    ASSERT_FALSE(mat.get<Cubemap>("cubemap").has_value());
}

TEST_F(Renderer, MaterialGetCubemapReturnsSomethingAfterSettingCubemap)
{
    Material mat = generate_material();

    ASSERT_FALSE(mat.get<Cubemap>("cubemap").has_value());

    const Cubemap cubemap{1, TextureFormat::RGBA32};

    mat.set("cubemap", cubemap);

    ASSERT_TRUE(mat.get<Cubemap>("cubemap").has_value());
}

TEST_F(Renderer, MaterialGetCubemapReturnsTheCubemapThatWasLastSet)
{
    Material mat = generate_material();

    ASSERT_FALSE(mat.get<Cubemap>("cubemap").has_value());

    const Cubemap firstCubemap{1, TextureFormat::RGBA32};
    const Cubemap secondCubemap{2, TextureFormat::RGBA32};  // different

    mat.set<Cubemap>("cubemap", firstCubemap);
    ASSERT_EQ(mat.get<Cubemap>("cubemap"), firstCubemap);

    mat.set<Cubemap>("cubemap", secondCubemap);
    ASSERT_EQ(mat.get<Cubemap>("cubemap"), secondCubemap);
}

TEST_F(Renderer, MaterialUnsetCubemapClearsTheCubemap)
{
    Material mat = generate_material();

    ASSERT_FALSE(mat.get<Cubemap>("cubemap").has_value());

    const Cubemap cubemap{1, TextureFormat::RGBA32};

    mat.set("cubemap", cubemap);

    ASSERT_TRUE(mat.get<Cubemap>("cubemap").has_value());

    mat.unset("cubemap");

    ASSERT_FALSE(mat.get<Cubemap>("cubemap").has_value());
}

TEST_F(Renderer, MaterialGetTransparentIsInitiallyFalse)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.is_transparent());
}

TEST_F(Renderer, MaterialSetTransparentBehavesAsExpected)
{
    Material mat = generate_material();
    mat.set_transparent(true);
    ASSERT_TRUE(mat.is_transparent());
    mat.set_transparent(false);
    ASSERT_FALSE(mat.is_transparent());
    mat.set_transparent(true);
    ASSERT_TRUE(mat.is_transparent());
}

TEST_F(Renderer, Material_source_blending_factor_returns_Default_when_not_set)
{
    const Material mat = generate_material();
    ASSERT_EQ(mat.source_blending_factor(), SourceBlendingFactor::Default);
}

TEST_F(Renderer, Material_set_source_blending_factor_sets_source_blending_factor)
{
    static_assert(SourceBlendingFactor::Default != SourceBlendingFactor::Zero);

    Material mat = generate_material();
    mat.set_source_blending_factor(SourceBlendingFactor::Zero);
    ASSERT_EQ(mat.source_blending_factor(), SourceBlendingFactor::Zero);
}

TEST_F(Renderer, Material_destination_blending_factor_returns_Default_when_not_set)
{
    const Material mat = generate_material();
    ASSERT_EQ(mat.destination_blending_factor(), DestinationBlendingFactor::Default);
}

TEST_F(Renderer, Material_set_destination_blending_factor_sets_destination_blending_factor)
{
    static_assert(DestinationBlendingFactor::Default != DestinationBlendingFactor::SourceAlpha);

    Material mat = generate_material();
    mat.set_destination_blending_factor(DestinationBlendingFactor::SourceAlpha);
    ASSERT_EQ(mat.destination_blending_factor(), DestinationBlendingFactor::SourceAlpha);
}

TEST_F(Renderer, Material_blending_equation_returns_Default_when_not_set)
{
    const Material mat = generate_material();
    ASSERT_EQ(mat.blending_equation(), BlendingEquation::Default);
}

TEST_F(Renderer, Material_set_blending_equation_sets_blending_equation)
{
    static_assert(BlendingEquation::Default != BlendingEquation::Max);

    Material mat = generate_material();
    mat.set_blending_equation(BlendingEquation::Max);
    ASSERT_EQ(mat.blending_equation(), BlendingEquation::Max);
}

TEST_F(Renderer, MaterialGetDepthTestedIsInitiallyTrue)
{
    Material mat = generate_material();
    ASSERT_TRUE(mat.is_depth_tested());
}

TEST_F(Renderer, MaterialSetDepthTestedBehavesAsExpected)
{
    Material mat = generate_material();
    mat.set_depth_tested(false);
    ASSERT_FALSE(mat.is_depth_tested());
    mat.set_depth_tested(true);
    ASSERT_TRUE(mat.is_depth_tested());
    mat.set_depth_tested(false);
    ASSERT_FALSE(mat.is_depth_tested());
}

TEST_F(Renderer, MaterialGetDepthFunctionIsInitiallyDefault)
{
    Material mat = generate_material();
    ASSERT_EQ(mat.depth_function(), DepthFunction::Default);
}

TEST_F(Renderer, MaterialSetDepthFunctionBehavesAsExpected)
{
    Material mat = generate_material();

    ASSERT_EQ(mat.depth_function(), DepthFunction::Default);

    static_assert(DepthFunction::Default != DepthFunction::LessOrEqual);

    mat.set_depth_function(DepthFunction::LessOrEqual);

    ASSERT_EQ(mat.depth_function(), DepthFunction::LessOrEqual);
}

TEST_F(Renderer, MaterialGetWireframeModeIsInitiallyFalse)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.is_wireframe());
}

TEST_F(Renderer, MaterialSetWireframeModeBehavesAsExpected)
{
    Material mat = generate_material();
    mat.set_wireframe(false);
    ASSERT_FALSE(mat.is_wireframe());
    mat.set_wireframe(true);
    ASSERT_TRUE(mat.is_wireframe());
    mat.set_wireframe(false);
    ASSERT_FALSE(mat.is_wireframe());
}

TEST_F(Renderer, MaterialSetWireframeModeCausesMaterialCopiesToReturnNonEqual)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.is_wireframe());
    Material copy{mat};
    ASSERT_EQ(mat, copy);
    copy.set_wireframe(true);
    ASSERT_NE(mat, copy);
}

TEST_F(Renderer, MaterialGetCullModeIsInitiallyDefault)
{
    Material mat = generate_material();
    ASSERT_EQ(mat.cull_mode(), CullMode::Default);
}

TEST_F(Renderer, MaterialSetCullModeBehavesAsExpected)
{
    Material mat = generate_material();

    constexpr CullMode newCullMode = CullMode::Front;

    ASSERT_NE(mat.cull_mode(), newCullMode);
    mat.set_cull_mode(newCullMode);
    ASSERT_EQ(mat.cull_mode(), newCullMode);
}

TEST_F(Renderer, MaterialSetCullModeCausesMaterialCopiesToBeNonEqual)
{
    constexpr CullMode newCullMode = CullMode::Front;

    Material mat = generate_material();
    Material copy{mat};

    ASSERT_EQ(mat, copy);
    ASSERT_NE(mat.cull_mode(), newCullMode);
    mat.set_cull_mode(newCullMode);
    ASSERT_NE(mat, copy);
}

TEST_F(Renderer, MaterialCanCompareEquals)
{
    Material mat = generate_material();
    Material copy{mat};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(mat, copy);
}

TEST_F(Renderer, MaterialCanCompareNotEquals)
{
    Material m1 = generate_material();
    Material m2 = generate_material();

    ASSERT_NE(m1, m2);
}

TEST_F(Renderer, MaterialCanPrintToStringStream)
{
    Material m1 = generate_material();

    std::stringstream ss;
    ss << m1;
}

TEST_F(Renderer, MaterialOutputStringContainsUsefulInformation)
{
    Material m1 = generate_material();
    std::stringstream ss;

    ss << m1;

    std::string str{ss.str()};

    ASSERT_TRUE(contains_case_insensitive(str, "Material"));

    // TODO: should print more useful info, such as number of props etc.
}

TEST_F(Renderer, MaterialSetFloatAndThenSetVec3CausesGetFloatToReturnEmpty)
{
    // compound test: when the caller sets a Vec3 then calling get_int with the same key should return empty
    Material mat = generate_material();

    std::string key = "someKey";
    float floatValue = generate<float>();
    Vec3 vecValue = generate<Vec3>();

    mat.set<float>(key, floatValue);

    ASSERT_TRUE(mat.get<float>(key));

    mat.set<Vec3>(key, vecValue);

    ASSERT_TRUE(mat.get<Vec3>(key));
    ASSERT_FALSE(mat.get<float>(key));
}
TEST_F(Renderer, MeshTopologyAllCanBeWrittenToStream)
{
    for (size_t i = 0; i < num_options<MeshTopology>(); ++i) {
        const auto mt = static_cast<MeshTopology>(i);

        std::stringstream ss;

        ss << mt;

        ASSERT_FALSE(ss.str().empty());
    }
}

TEST_F(Renderer, LoadTexture2DFromImageResourceCanLoadImageFile)
{
    const Texture2D t = load_texture2D_from_image(
        App::load_resource((std::filesystem::path{OSC_TESTING_RESOURCES_DIR} / "awesomeface.png").string()),
        ColorSpace::sRGB
    );
    ASSERT_EQ(t.dimensions(), Vec2i(512, 512));
}

TEST_F(Renderer, LoadTexture2DFromImageResourceThrowsIfResourceNotFound)
{
    ASSERT_ANY_THROW(
    {
        load_texture2D_from_image(
            App::load_resource("textures/doesnt_exist.png"),
            ColorSpace::sRGB
        );
    });
}

TEST_F(Renderer, DrawMeshDoesNotThrowWithStandardArgs)
{
    const Mesh mesh;
    const Transform transform = identity<Transform>();
    const Material material = generate_material();
    Camera camera;

    ASSERT_NO_THROW({ graphics::draw(mesh, transform, material, camera); });
}

TEST_F(Renderer, DrawMeshThrowsIfGivenOutOfBoundsSubMeshIndex)
{
    const Mesh mesh;
    const Transform transform = identity<Transform>();
    const Material material = generate_material();
    Camera camera;

    ASSERT_ANY_THROW({ graphics::draw(mesh, transform, material, camera, std::nullopt, 0); });
}

TEST_F(Renderer, DrawMeshDoesNotThrowIfGivenInBoundsSubMesh)
{
    Mesh mesh;
    mesh.push_submesh_descriptor({0, 0, MeshTopology::Triangles});
    const Transform transform = identity<Transform>();
    const Material material = generate_material();
    Camera camera;

    ASSERT_NO_THROW({ graphics::draw(mesh, transform, material, camera, std::nullopt, 0); });
}

TEST_F(Renderer, MeshDepthWritingMaterial_can_default_construct)
{
    [[maybe_unused]] MeshDepthWritingMaterial default_constructed;  // should compile, run, etc.
}

TEST_F(Renderer, MeshNormalVectorsMaterial_can_default_construct)
{
    [[maybe_unused]] MeshNormalVectorsMaterial default_constructed;  // should compile, run, etc.
}
