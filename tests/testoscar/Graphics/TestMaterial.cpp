#include <oscar/Graphics/Material.h>

#include <testoscar/TestingHelpers.h>
#include <testoscar/testoscarconfig.h>

#include <gtest/gtest.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppMetadata.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StringHelpers.h>

#include <array>
#include <memory>

using namespace osc;
using namespace osc::testing;

namespace
{
    std::unique_ptr<App> g_material_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    class MaterialTest : public ::testing::Test {
    protected:
        static void SetUpTestSuite()
        {
            const AppMetadata metadata{TESTOSCAR_ORGNAME_STRING, TESTOSCAR_APPNAME_STRING};
            g_material_app = std::make_unique<App>(metadata);
        }

        static void TearDownTestSuite()
        {
            g_material_app.reset();
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

    Texture2D generate_red_texture()
    {
        Texture2D rv{Vec2i{2, 2}};
        rv.set_pixels(std::to_array({ Color::red(), Color::red(), Color::red(), Color::red() }));
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

TEST_F(MaterialTest, can_be_constructed_from_a_shader)
{
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};
    const Material material{shader};
}

TEST_F(MaterialTest, can_be_copy_constructed)
{
    const Material material = generate_material();
    const Material copy{material}; // NOLINT(performance-unnecessary-copy-initialization)
}

TEST_F(MaterialTest, can_be_move_constructed)
{
    Material material = generate_material();
    const Material move_constructed{std::move(material)};
}

TEST_F(MaterialTest, can_be_copy_assigned)
{
    Material lhs = generate_material();
    const Material rhs = generate_material();

    lhs = rhs;
}

TEST_F(MaterialTest, can_be_move_assigned)
{
    Material lhs = generate_material();
    Material rhs = generate_material();

    lhs = std::move(rhs);
}

TEST_F(MaterialTest, copy_constructed_instance_compares_equal_to_copied_from_instance)
{
    const Material material = generate_material();
    const Material copy{material}; // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(material, copy);
}

TEST_F(MaterialTest, copy_assigned_instance_compares_equal_to_copied_from_instance)
{
    Material lhs = generate_material();
    const Material rhs = generate_material();

    ASSERT_NE(lhs, rhs);
    lhs = rhs;
    ASSERT_EQ(lhs, rhs);
}

TEST_F(MaterialTest, shader_returns_the_Shader_supplied_via_the_constructor)
{
    const Shader shader{c_vertex_shader_src, c_fragment_shader_src};
    const Material material{shader};

    ASSERT_EQ(material.shader(), shader);
}

TEST_F(MaterialTest, get_Color_on_new_instance_returns_nullopt)
{
    const Material material = generate_material();

    ASSERT_FALSE(material.get<Color>("someKey"));
}

TEST_F(MaterialTest, can_call_set_Color_on_new_instance)
{
    Material material = generate_material();
    material.set<Color>("someKey", Color::red());
}

TEST_F(MaterialTest, set_Color_makes_get_Color_return_the_Color)
{
    Material material = generate_material();
    material.set<Color>("someKey", Color::red());

    ASSERT_EQ(material.get<Color>("someKey"), Color::red());
}

TEST_F(MaterialTest, MaterialGetColorArrayReturnsEmptyOnNewMaterial)
{
    Material mat = generate_material();

    ASSERT_FALSE(mat.get_array<Color>("someKey"));
}

TEST_F(MaterialTest, MaterialCanCallSetColorArrayOnNewMaterial)
{
    Material mat = generate_material();
    const auto colors = std::to_array({Color::black(), Color::blue()});

    mat.set_array<Color>("someKey", colors);
}

TEST_F(MaterialTest, MaterialCallingGetColorArrayOnMaterialAfterSettingThemReturnsTheSameColors)
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

TEST_F(MaterialTest, MaterialGetFloatOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<float>("someKey"));
}

TEST_F(MaterialTest, MaterialGetFloatArrayOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get_array<float>("someKey"));
}

TEST_F(MaterialTest, MaterialGetVec2OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Vec2>("someKey"));
}

TEST_F(MaterialTest, MaterialGetVec3OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Vec3>("someKey"));
}

TEST_F(MaterialTest, MaterialGetVec3ArrayOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get_array<Vec3>("someKey"));
}

TEST_F(MaterialTest, MaterialGetVec4OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Vec4>("someKey"));
}

TEST_F(MaterialTest, MaterialGetMat3OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Mat3>("someKey"));
}

TEST_F(MaterialTest, MaterialGetMat4OnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<Mat4>("someKey"));
}

TEST_F(MaterialTest, MaterialGetIntOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<int>("someKey"));
}

TEST_F(MaterialTest, MaterialGetBoolOnNewMaterialReturnsEmptyOptional)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get<bool>("someKey"));
}

TEST_F(MaterialTest, MaterialSetFloatOnMaterialCausesGetFloatToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    float value = generate<float>();

    mat.set<float>(key, value);

    ASSERT_EQ(*mat.get<float>(key), value);
}

TEST_F(MaterialTest, MaterialSetFloatArrayOnMaterialCausesGetFloatArrayToReturnTheProvidedValues)
{
    Material mat = generate_material();
    std::string key = "someKey";
    std::array<float, 4> values = {generate<float>(), generate<float>(), generate<float>(), generate<float>()};

    ASSERT_FALSE(mat.get_array<float>(key));

    mat.set_array<float>(key, values);

    std::span<const float> rv = mat.get_array<float>(key).value();
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(MaterialTest, MaterialSetVec2OnMaterialCausesGetVec2ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Vec2 value = generate<Vec2>();

    mat.set<Vec2>(key, value);

    ASSERT_EQ(*mat.get<Vec2>(key), value);
}

TEST_F(MaterialTest, MaterialSetVec2AndThenSetVec3CausesGetVec2ToReturnEmpty)
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

TEST_F(MaterialTest, MaterialSetVec2CausesMaterialToCompareNotEqualToCopy)
{
    Material mat = generate_material();
    Material copy{mat};

    mat.set<Vec2>("someKey", generate<Vec2>());

    ASSERT_NE(mat, copy);
}

TEST_F(MaterialTest, MaterialSetVec3OnMaterialCausesGetVec3ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Vec3 value = generate<Vec3>();

    mat.set<Vec3>(key, value);

    ASSERT_EQ(*mat.get<Vec3>(key), value);
}

TEST_F(MaterialTest, MaterialSetVec3ArrayOnMaterialCausesGetVec3ArrayToReutrnTheProvidedValues)
{
    Material mat = generate_material();
    std::string key = "someKey";
    std::array<Vec3, 4> values = {generate<Vec3>(), generate<Vec3>(), generate<Vec3>(), generate<Vec3>()};

    ASSERT_FALSE(mat.get_array<Vec3>(key));

    mat.set_array<Vec3>(key, values);

    std::span<const Vec3> rv = mat.get_array<Vec3>(key).value();
    ASSERT_TRUE(std::equal(rv.begin(), rv.end(), values.begin(), values.end()));
}

TEST_F(MaterialTest, MaterialSetVec4OnMaterialCausesGetVec4ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Vec4 value = generate<Vec4>();

    mat.set<Vec4>(key, value);

    ASSERT_EQ(*mat.get<Vec4>(key), value);
}

TEST_F(MaterialTest, MaterialSetMat3OnMaterialCausesGetMat3ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Mat3 value = generate<Mat3>();

    mat.set<Mat3>(key, value);

    ASSERT_EQ(*mat.get<Mat3>(key), value);
}

TEST_F(MaterialTest, MaterialSetMat4OnMaterialCausesGetMat4ToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Mat4 value = generate<Mat4>();

    mat.set<Mat4>(key, value);

    ASSERT_EQ(*mat.get<Mat4>(key), value);
}

TEST_F(MaterialTest, MaterialGetMat4ArrayInitiallyReturnsNothing)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.get_array<Mat4>("someKey").has_value());
}

TEST_F(MaterialTest, MaterialSetMat4ArrayCausesGetMat4ArrayToReturnSameSequenceOfValues)
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

TEST_F(MaterialTest, MaterialSetIntOnMaterialCausesGetIntToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    int value = generate<int>();

    mat.set<int>(key, value);

    ASSERT_EQ(*mat.get<int>(key), value);
}

TEST_F(MaterialTest, MaterialSetBoolOnMaterialCausesGetBoolToReturnTheProvidedValue)
{
    Material mat = generate_material();

    std::string key = "someKey";
    bool value = generate<bool>();

    mat.set<bool>(key, value);

    ASSERT_EQ(*mat.get<bool>(key), value);
}

TEST_F(MaterialTest, MaterialSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    Material mat = generate_material();

    std::string key = "someKey";
    Texture2D t = generate_red_texture();

    ASSERT_FALSE(mat.get<Texture2D>(key));

    mat.set(key, t);

    ASSERT_TRUE(mat.get<Texture2D>(key));
}

TEST_F(MaterialTest, MaterialUnsetTextureOnMaterialCausesGetTextureToReturnNothing)
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

TEST_F(MaterialTest, MaterialSetRenderTextureCausesGetRenderTextureToReturnTheTexture)
{
    Material mat = generate_material();
    RenderTexture renderTex = generate_render_texture();
    std::string key = "someKey";

    ASSERT_FALSE(mat.get<RenderTexture>(key));

    mat.set(key, renderTex);

    ASSERT_EQ(*mat.get<RenderTexture>(key), renderTex);
}

TEST_F(MaterialTest, MaterialSetRenderTextureFollowedByUnsetClearsTheRenderTexture)
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

TEST_F(MaterialTest, MaterialGetCubemapInitiallyReturnsNothing)
{
    const Material mat = generate_material();

    ASSERT_FALSE(mat.get<Cubemap>("cubemap").has_value());
}

TEST_F(MaterialTest, MaterialGetCubemapReturnsSomethingAfterSettingCubemap)
{
    Material mat = generate_material();

    ASSERT_FALSE(mat.get<Cubemap>("cubemap").has_value());

    const Cubemap cubemap{1, TextureFormat::RGBA32};

    mat.set("cubemap", cubemap);

    ASSERT_TRUE(mat.get<Cubemap>("cubemap").has_value());
}

TEST_F(MaterialTest, MaterialGetCubemapReturnsTheCubemapThatWasLastSet)
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

TEST_F(MaterialTest, MaterialUnsetCubemapClearsTheCubemap)
{
    Material mat = generate_material();

    ASSERT_FALSE(mat.get<Cubemap>("cubemap").has_value());

    const Cubemap cubemap{1, TextureFormat::RGBA32};

    mat.set("cubemap", cubemap);

    ASSERT_TRUE(mat.get<Cubemap>("cubemap").has_value());

    mat.unset("cubemap");

    ASSERT_FALSE(mat.get<Cubemap>("cubemap").has_value());
}

TEST_F(MaterialTest, MaterialGetTransparentIsInitiallyFalse)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.is_transparent());
}

TEST_F(MaterialTest, MaterialSetTransparentBehavesAsExpected)
{
    Material mat = generate_material();
    mat.set_transparent(true);
    ASSERT_TRUE(mat.is_transparent());
    mat.set_transparent(false);
    ASSERT_FALSE(mat.is_transparent());
    mat.set_transparent(true);
    ASSERT_TRUE(mat.is_transparent());
}

TEST_F(MaterialTest, Material_source_blending_factor_returns_Default_when_not_set)
{
    const Material mat = generate_material();
    ASSERT_EQ(mat.source_blending_factor(), SourceBlendingFactor::Default);
}

TEST_F(MaterialTest, Material_set_source_blending_factor_sets_source_blending_factor)
{
    static_assert(SourceBlendingFactor::Default != SourceBlendingFactor::Zero);

    Material mat = generate_material();
    mat.set_source_blending_factor(SourceBlendingFactor::Zero);
    ASSERT_EQ(mat.source_blending_factor(), SourceBlendingFactor::Zero);
}

TEST_F(MaterialTest, Material_destination_blending_factor_returns_Default_when_not_set)
{
    const Material mat = generate_material();
    ASSERT_EQ(mat.destination_blending_factor(), DestinationBlendingFactor::Default);
}

TEST_F(MaterialTest, Material_set_destination_blending_factor_sets_destination_blending_factor)
{
    static_assert(DestinationBlendingFactor::Default != DestinationBlendingFactor::SourceAlpha);

    Material mat = generate_material();
    mat.set_destination_blending_factor(DestinationBlendingFactor::SourceAlpha);
    ASSERT_EQ(mat.destination_blending_factor(), DestinationBlendingFactor::SourceAlpha);
}

TEST_F(MaterialTest, Material_blending_equation_returns_Default_when_not_set)
{
    const Material mat = generate_material();
    ASSERT_EQ(mat.blending_equation(), BlendingEquation::Default);
}

TEST_F(MaterialTest, Material_set_blending_equation_sets_blending_equation)
{
    static_assert(BlendingEquation::Default != BlendingEquation::Max);

    Material mat = generate_material();
    mat.set_blending_equation(BlendingEquation::Max);
    ASSERT_EQ(mat.blending_equation(), BlendingEquation::Max);
}

TEST_F(MaterialTest, MaterialGetDepthTestedIsInitiallyTrue)
{
    Material mat = generate_material();
    ASSERT_TRUE(mat.is_depth_tested());
}

TEST_F(MaterialTest, MaterialSetDepthTestedBehavesAsExpected)
{
    Material mat = generate_material();
    mat.set_depth_tested(false);
    ASSERT_FALSE(mat.is_depth_tested());
    mat.set_depth_tested(true);
    ASSERT_TRUE(mat.is_depth_tested());
    mat.set_depth_tested(false);
    ASSERT_FALSE(mat.is_depth_tested());
}

TEST_F(MaterialTest, MaterialGetDepthFunctionIsInitiallyDefault)
{
    Material mat = generate_material();
    ASSERT_EQ(mat.depth_function(), DepthFunction::Default);
}

TEST_F(MaterialTest, MaterialSetDepthFunctionBehavesAsExpected)
{
    Material mat = generate_material();

    ASSERT_EQ(mat.depth_function(), DepthFunction::Default);

    static_assert(DepthFunction::Default != DepthFunction::LessOrEqual);

    mat.set_depth_function(DepthFunction::LessOrEqual);

    ASSERT_EQ(mat.depth_function(), DepthFunction::LessOrEqual);
}

TEST_F(MaterialTest, MaterialGetWireframeModeIsInitiallyFalse)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.is_wireframe());
}

TEST_F(MaterialTest, MaterialSetWireframeModeBehavesAsExpected)
{
    Material mat = generate_material();
    mat.set_wireframe(false);
    ASSERT_FALSE(mat.is_wireframe());
    mat.set_wireframe(true);
    ASSERT_TRUE(mat.is_wireframe());
    mat.set_wireframe(false);
    ASSERT_FALSE(mat.is_wireframe());
}

TEST_F(MaterialTest, MaterialSetWireframeModeCausesMaterialCopiesToReturnNonEqual)
{
    Material mat = generate_material();
    ASSERT_FALSE(mat.is_wireframe());
    Material copy{mat};
    ASSERT_EQ(mat, copy);
    copy.set_wireframe(true);
    ASSERT_NE(mat, copy);
}

TEST_F(MaterialTest, MaterialGetCullModeIsInitiallyDefault)
{
    Material mat = generate_material();
    ASSERT_EQ(mat.cull_mode(), CullMode::Default);
}

TEST_F(MaterialTest, MaterialSetCullModeBehavesAsExpected)
{
    Material mat = generate_material();

    constexpr CullMode newCullMode = CullMode::Front;

    ASSERT_NE(mat.cull_mode(), newCullMode);
    mat.set_cull_mode(newCullMode);
    ASSERT_EQ(mat.cull_mode(), newCullMode);
}

TEST_F(MaterialTest, MaterialSetCullModeCausesMaterialCopiesToBeNonEqual)
{
    constexpr CullMode newCullMode = CullMode::Front;

    Material mat = generate_material();
    Material copy{mat};

    ASSERT_EQ(mat, copy);
    ASSERT_NE(mat.cull_mode(), newCullMode);
    mat.set_cull_mode(newCullMode);
    ASSERT_NE(mat, copy);
}

TEST_F(MaterialTest, MaterialCanCompareEquals)
{
    Material mat = generate_material();
    Material copy{mat};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(mat, copy);
}

TEST_F(MaterialTest, MaterialCanCompareNotEquals)
{
    Material m1 = generate_material();
    Material m2 = generate_material();

    ASSERT_NE(m1, m2);
}

TEST_F(MaterialTest, MaterialCanPrintToStringStream)
{
    Material m1 = generate_material();

    std::stringstream ss;
    ss << m1;
}

TEST_F(MaterialTest, MaterialOutputStringContainsUsefulInformation)
{
    Material m1 = generate_material();
    std::stringstream ss;

    ss << m1;

    std::string str{ss.str()};

    ASSERT_TRUE(contains_case_insensitive(str, "Material"));

    // TODO: should print more useful info, such as number of props etc.
}

TEST_F(MaterialTest, MaterialSetFloatAndThenSetVec3CausesGetFloatToReturnEmpty)
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
