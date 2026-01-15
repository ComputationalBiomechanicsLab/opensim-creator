#include "material.h"

#include <liboscar/graphics/render_texture.h>
#include <liboscar/graphics/texture2_d.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/app_metadata.h>
#include <liboscar/tests/test_helpers.h>
#include <liboscar/tests/testoscarconfig.h>
#include <liboscar/utils/c_string_view.h>
#include <liboscar/utils/string_helpers.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <memory>
#include <ranges>

using namespace osc;
using namespace osc::tests;
namespace rgs = std::ranges;

namespace
{
    std::unique_ptr<App> g_material_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    class MaterialTest : public ::testing::Test {
    protected:
        static void SetUpTestSuite()
        {
            AppMetadata metadata;
            metadata.set_organization_name(TESTOSCAR_ORGNAME_STRING);
            metadata.set_application_name(TESTOSCAR_APPNAME_STRING);
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
            // don't need this un-projection math trick

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
        Texture2D rv{Vector2i{2, 2}};
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
        return RenderTexture{{.pixel_dimensions = {2, 2}}};
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
    const Material copy{material};  // NOLINT(performance-unnecessary-copy-initialization)
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
    const Material material = generate_material();

    ASSERT_FALSE(material.get_array<Color>("someKey"));
}

TEST_F(MaterialTest, MaterialCanCallSetColorArrayOnNewMaterial)
{
    Material material = generate_material();
    const auto colors = std::to_array({Color::black(), Color::blue()});

    material.set_array<Color>("someKey", colors);
}

TEST_F(MaterialTest, MaterialCallingGetColorArrayOnMaterialAfterSettingThemReturnsTheSameColors)
{
    Material material = generate_material();
    const auto colors = std::to_array({Color::red(), Color::green(), Color::blue()});
    const CStringView key = "someKey";

    material.set_array<Color>(key, colors);

    const std::optional<std::span<const Color>> rv = material.get_array<Color>(key);

    ASSERT_TRUE(rv);
    ASSERT_EQ(std::size(*rv), std::size(colors));
    ASSERT_TRUE(std::equal(std::begin(colors), std::end(colors), std::begin(*rv)));
}

TEST_F(MaterialTest, MaterialGetFloatOnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get<float>("someKey"));
}

TEST_F(MaterialTest, MaterialGetFloatArrayOnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get_array<float>("someKey"));
}

TEST_F(MaterialTest, MaterialGetVector2OnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get<Vector2>("someKey"));
}

TEST_F(MaterialTest, MaterialGetVector3OnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get<Vector3>("someKey"));
}

TEST_F(MaterialTest, MaterialGetVector3ArrayOnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get_array<Vector3>("someKey"));
}

TEST_F(MaterialTest, MaterialGetVector4OnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get<Vector4>("someKey"));
}

TEST_F(MaterialTest, MaterialGetMatrix3x3OnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get<Matrix3x3>("someKey"));
}

TEST_F(MaterialTest, MaterialGetMatrix4x4OnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get<Matrix4x4>("someKey"));
}

TEST_F(MaterialTest, MaterialGetIntOnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get<int>("someKey"));
}

TEST_F(MaterialTest, MaterialGetBoolOnNewMaterialReturnsEmptyOptional)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get<bool>("someKey"));
}

TEST_F(MaterialTest, MaterialSetFloatOnMaterialCausesGetFloatToReturnTheProvidedValue)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const float value = generate<float>();

    material.set<float>(key, value);

    ASSERT_EQ(*material.get<float>(key), value);
}

TEST_F(MaterialTest, MaterialSetFloatArrayOnMaterialCausesGetFloatArrayToReturnTheProvidedValues)
{
    Material material = generate_material();
    const std::string key = "someKey";
    const auto values = std::to_array({ generate<float>(), generate<float>(), generate<float>(), generate<float>() });

    ASSERT_FALSE(material.get_array<float>(key));

    material.set_array<float>(key, values);

    const std::span<const float> rv = material.get_array<float>(key).value();
    ASSERT_TRUE(rgs::equal(rv, values));
}

TEST_F(MaterialTest, MaterialSetVector2OnMaterialCausesGetVector2ToReturnTheProvidedValue)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const Vector2 value = generate<Vector2>();

    material.set<Vector2>(key, value);

    ASSERT_EQ(*material.get<Vector2>(key), value);
}

TEST_F(MaterialTest, MaterialSetVector2AndThenSetVector3CausesGetVector2ToReturnEmpty)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const Vector2 value = generate<Vector2>();

    ASSERT_FALSE(material.get<Vector2>(key).has_value());

    material.set<Vector2>(key, value);

    ASSERT_TRUE(material.get<Vector2>(key).has_value());

    material.set<Vector3>(key, {});

    ASSERT_TRUE(material.get<Vector3>(key));
    ASSERT_FALSE(material.get<Vector2>(key));
}

TEST_F(MaterialTest, MaterialSetVector2CausesMaterialToCompareNotEqualToCopy)
{
    Material material = generate_material();
    const Material copy{material};

    material.set<Vector2>("someKey", generate<Vector2>());

    ASSERT_NE(material, copy);
}

TEST_F(MaterialTest, MaterialSetVector3OnMaterialCausesGetVector3ToReturnTheProvidedValue)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const Vector3 value = generate<Vector3>();

    material.set<Vector3>(key, value);

    ASSERT_EQ(*material.get<Vector3>(key), value);
}

TEST_F(MaterialTest, MaterialSetVector3ArrayOnMaterialCausesGetVector3ArrayToReutrnTheProvidedValues)
{
    Material material = generate_material();
    const std::string key = "someKey";
    const auto values = std::to_array({ generate<Vector3>(), generate<Vector3>(), generate<Vector3>(), generate<Vector3>() });

    ASSERT_FALSE(material.get_array<Vector3>(key));

    material.set_array<Vector3>(key, values);

    const std::span<const Vector3> rv = material.get_array<Vector3>(key).value();
    ASSERT_TRUE(rgs::equal(rv, values));
}

TEST_F(MaterialTest, MaterialSetVector4OnMaterialCausesGetVector4ToReturnTheProvidedValue)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const Vector4 value = generate<Vector4>();

    material.set<Vector4>(key, value);

    ASSERT_EQ(*material.get<Vector4>(key), value);
}

TEST_F(MaterialTest, MaterialSetMat3OnMaterialCausesGetMat3ToReturnTheProvidedValue)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const Matrix3x3 value = generate<Matrix3x3>();

    material.set<Matrix3x3>(key, value);

    ASSERT_EQ(*material.get<Matrix3x3>(key), value);
}

TEST_F(MaterialTest, MaterialSetMatrix4x4OnMaterialCausesGetMat4ToReturnTheProvidedValue)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const Matrix4x4 value = generate<Matrix4x4>();

    material.set<Matrix4x4>(key, value);

    ASSERT_EQ(*material.get<Matrix4x4>(key), value);
}

TEST_F(MaterialTest, MaterialGetMatrix4x4ArrayInitiallyReturnsNothing)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.get_array<Matrix4x4>("someKey").has_value());
}

TEST_F(MaterialTest, MaterialSetMatrix4x4ArrayCausesGetMat4ArrayToReturnSameSequenceOfValues)
{
    const auto matrix4x4_array = std::to_array<Matrix4x4>({
        generate<Matrix4x4>(),
        generate<Matrix4x4>(),
        generate<Matrix4x4>(),
        generate<Matrix4x4>()
    });

    Material material = generate_material();
    material.set_array<Matrix4x4>("someKey", matrix4x4_array);

    std::optional<std::span<const Matrix4x4>> rv = material.get_array<Matrix4x4>("someKey");
    ASSERT_TRUE(rv.has_value());
    ASSERT_EQ(matrix4x4_array.size(), rv->size());
    ASSERT_TRUE(std::equal(matrix4x4_array.begin(), matrix4x4_array.end(), rv->begin()));
}

TEST_F(MaterialTest, MaterialSetIntOnMaterialCausesGetIntToReturnTheProvidedValue)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const int value = generate<int>();

    material.set<int>(key, value);

    ASSERT_EQ(*material.get<int>(key), value);
}

TEST_F(MaterialTest, MaterialSetBoolOnMaterialCausesGetBoolToReturnTheProvidedValue)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const bool value = generate<bool>();

    material.set<bool>(key, value);

    ASSERT_EQ(*material.get<bool>(key), value);
}

TEST_F(MaterialTest, MaterialSetTextureOnMaterialCausesGetTextureToReturnTheTexture)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const Texture2D texture_2d = generate_red_texture();

    ASSERT_FALSE(material.get<Texture2D>(key));

    material.set(key, texture_2d);

    ASSERT_TRUE(material.get<Texture2D>(key));
}

TEST_F(MaterialTest, MaterialUnsetTextureOnMaterialCausesGetTextureToReturnNothing)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const Texture2D texture_2d = generate_red_texture();

    ASSERT_FALSE(material.get<Texture2D>(key));

    material.set(key, texture_2d);

    ASSERT_TRUE(material.get<Texture2D>(key));

    material.unset(key);

    ASSERT_FALSE(material.get<Texture2D>(key));
}

TEST_F(MaterialTest, MaterialSetRenderTextureCausesGetRenderTextureToReturnTheTexture)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const RenderTexture render_texture = generate_render_texture();

    ASSERT_FALSE(material.get<RenderTexture>(key));

    material.set(key, render_texture);

    ASSERT_EQ(*material.get<RenderTexture>(key), render_texture);
}

TEST_F(MaterialTest, MaterialSetRenderTextureFollowedByUnsetClearsTheRenderTexture)
{
    Material material = generate_material();

    const std::string key = "someKey";
    const RenderTexture render_texture = generate_render_texture();

    ASSERT_FALSE(material.get<RenderTexture>(key));

    material.set(key, render_texture);

    ASSERT_EQ(*material.get<RenderTexture>(key), render_texture);

    material.unset(key);

    ASSERT_FALSE(material.get<RenderTexture>(key));
}

TEST_F(MaterialTest, MaterialGetCubemapInitiallyReturnsNothing)
{
    const Material material = generate_material();

    ASSERT_FALSE(material.get<Cubemap>("cubemap").has_value());
}

TEST_F(MaterialTest, MaterialGetCubemapReturnsSomethingAfterSettingCubemap)
{
    Material material = generate_material();

    ASSERT_FALSE(material.get<Cubemap>("cubemap").has_value());

    const Cubemap cubemap{1, TextureFormat::RGBA32};

    material.set("cubemap", cubemap);

    ASSERT_TRUE(material.get<Cubemap>("cubemap").has_value());
}

TEST_F(MaterialTest, MaterialGetCubemapReturnsTheCubemapThatWasLastSet)
{
    Material material = generate_material();

    ASSERT_FALSE(material.get<Cubemap>("cubemap").has_value());

    const Cubemap first_cubemap{1, TextureFormat::RGBA32};
    const Cubemap second_cubemap{2, TextureFormat::RGBA32};  // different

    material.set<Cubemap>("cubemap", first_cubemap);
    ASSERT_EQ(material.get<Cubemap>("cubemap"), first_cubemap);

    material.set<Cubemap>("cubemap", second_cubemap);
    ASSERT_EQ(material.get<Cubemap>("cubemap"), second_cubemap);
}

TEST_F(MaterialTest, MaterialUnsetCubemapClearsTheCubemap)
{
    Material material = generate_material();

    ASSERT_FALSE(material.get<Cubemap>("cubemap").has_value());

    const Cubemap cubemap{1, TextureFormat::RGBA32};

    material.set("cubemap", cubemap);

    ASSERT_TRUE(material.get<Cubemap>("cubemap").has_value());

    material.unset("cubemap");

    ASSERT_FALSE(material.get<Cubemap>("cubemap").has_value());
}

TEST_F(MaterialTest, MaterialGetTransparentIsInitiallyFalse)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.is_transparent());
}

TEST_F(MaterialTest, MaterialSetTransparentBehavesAsExpected)
{
    Material material = generate_material();
    material.set_transparent(true);
    ASSERT_TRUE(material.is_transparent());
    material.set_transparent(false);
    ASSERT_FALSE(material.is_transparent());
    material.set_transparent(true);
    ASSERT_TRUE(material.is_transparent());
}

TEST_F(MaterialTest, Material_source_blending_factor_returns_Default_when_not_set)
{
    const Material material = generate_material();
    ASSERT_EQ(material.source_blending_factor(), SourceBlendingFactor::Default);
}

TEST_F(MaterialTest, Material_set_source_blending_factor_sets_source_blending_factor)
{
    static_assert(SourceBlendingFactor::Default != SourceBlendingFactor::Zero);

    Material material = generate_material();
    material.set_source_blending_factor(SourceBlendingFactor::Zero);
    ASSERT_EQ(material.source_blending_factor(), SourceBlendingFactor::Zero);
}

TEST_F(MaterialTest, Material_destination_blending_factor_returns_Default_when_not_set)
{
    const Material material = generate_material();
    ASSERT_EQ(material.destination_blending_factor(), DestinationBlendingFactor::Default);
}

TEST_F(MaterialTest, Material_set_destination_blending_factor_sets_destination_blending_factor)
{
    static_assert(DestinationBlendingFactor::Default != DestinationBlendingFactor::SourceAlpha);

    Material material = generate_material();
    material.set_destination_blending_factor(DestinationBlendingFactor::SourceAlpha);
    ASSERT_EQ(material.destination_blending_factor(), DestinationBlendingFactor::SourceAlpha);
}

TEST_F(MaterialTest, Material_blending_equation_returns_Default_when_not_set)
{
    const Material material = generate_material();
    ASSERT_EQ(material.blending_equation(), BlendingEquation::Default);
}

TEST_F(MaterialTest, Material_set_blending_equation_sets_blending_equation)
{
    static_assert(BlendingEquation::Default != BlendingEquation::Max);

    Material material = generate_material();
    material.set_blending_equation(BlendingEquation::Max);
    ASSERT_EQ(material.blending_equation(), BlendingEquation::Max);
}

TEST_F(MaterialTest, MaterialGetDepthTestedIsInitiallyTrue)
{
    const Material material = generate_material();
    ASSERT_TRUE(material.is_depth_tested());
}

TEST_F(MaterialTest, MaterialSetDepthTestedBehavesAsExpected)
{
    Material material = generate_material();
    material.set_depth_tested(false);
    ASSERT_FALSE(material.is_depth_tested());
    material.set_depth_tested(true);
    ASSERT_TRUE(material.is_depth_tested());
    material.set_depth_tested(false);
    ASSERT_FALSE(material.is_depth_tested());
}

TEST_F(MaterialTest, MaterialGetDepthFunctionIsInitiallyDefault)
{
    const Material material = generate_material();
    ASSERT_EQ(material.depth_function(), DepthFunction::Default);
}

TEST_F(MaterialTest, MaterialWritesToDepthBufferIsInitiallyTrue)
{
    const Material material = generate_material();
    ASSERT_TRUE(material.writes_to_depth_buffer());
}

TEST_F(MaterialTest, MaterialSetWritesToDepthBufferChangesWritesToDepthBuffer)
{
    Material material = generate_material();
    ASSERT_TRUE(material.writes_to_depth_buffer());
    material.set_writes_to_depth_buffer(false);
    ASSERT_FALSE(material.writes_to_depth_buffer());
    material.set_writes_to_depth_buffer(true);
    ASSERT_TRUE(material.writes_to_depth_buffer());
}

TEST_F(MaterialTest, MaterialSetDepthFunctionBehavesAsExpected)
{
    Material material = generate_material();

    ASSERT_EQ(material.depth_function(), DepthFunction::Default);

    static_assert(DepthFunction::Default != DepthFunction::LessOrEqual);

    material.set_depth_function(DepthFunction::LessOrEqual);

    ASSERT_EQ(material.depth_function(), DepthFunction::LessOrEqual);
}

TEST_F(MaterialTest, MaterialGetWireframeModeIsInitiallyFalse)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.is_wireframe());
}

TEST_F(MaterialTest, MaterialSetWireframeModeBehavesAsExpected)
{
    Material material = generate_material();
    material.set_wireframe(false);
    ASSERT_FALSE(material.is_wireframe());
    material.set_wireframe(true);
    ASSERT_TRUE(material.is_wireframe());
    material.set_wireframe(false);
    ASSERT_FALSE(material.is_wireframe());
}

TEST_F(MaterialTest, MaterialSetWireframeModeCausesMaterialCopiesToReturnNonEqual)
{
    const Material material = generate_material();
    ASSERT_FALSE(material.is_wireframe());
    Material copy{material};
    ASSERT_EQ(material, copy);
    copy.set_wireframe(true);
    ASSERT_NE(material, copy);
}

TEST_F(MaterialTest, MaterialGetCullModeIsInitiallyDefault)
{
    const Material material = generate_material();
    ASSERT_EQ(material.cull_mode(), CullMode::Default);
}

TEST_F(MaterialTest, MaterialSetCullModeBehavesAsExpected)
{
    Material material = generate_material();

    constexpr CullMode new_cull_mode = CullMode::Front;

    ASSERT_NE(material.cull_mode(), new_cull_mode);
    material.set_cull_mode(new_cull_mode);
    ASSERT_EQ(material.cull_mode(), new_cull_mode);
}

TEST_F(MaterialTest, MaterialSetCullModeCausesMaterialCopiesToBeNonEqual)
{
    constexpr CullMode new_cull_mode = CullMode::Front;

    Material material = generate_material();
    const Material material_copy{material};

    ASSERT_EQ(material, material_copy);
    ASSERT_NE(material.cull_mode(), new_cull_mode);
    material.set_cull_mode(new_cull_mode);
    ASSERT_NE(material, material_copy);
}

TEST_F(MaterialTest, MaterialCanCompareEquals)
{
    const Material material = generate_material();
    const Material material_copy{material};  // NOLINT(performance-unnecessary-copy-initialization)

    ASSERT_EQ(material, material_copy);
}

TEST_F(MaterialTest, MaterialCanCompareNotEquals)
{
    const Material material_a = generate_material();
    const Material material_b = generate_material();

    ASSERT_NE(material_a, material_b);
}

TEST_F(MaterialTest, MaterialCanPrintToStringStream)
{
    const Material material = generate_material();

    std::stringstream ss;
    ss << material;
}

TEST_F(MaterialTest, MaterialOutputStringContainsUsefulInformation)
{
    const Material material = generate_material();
    std::stringstream ss;

    ss << material;

    const std::string str{ss.str()};

    ASSERT_TRUE(contains_case_insensitive(str, "Material"));

    // TODO: should print more useful info, such as number of props etc.
}

TEST_F(MaterialTest, MaterialSetFloatAndThenSetVector3CausesGetFloatToReturnEmpty)
{
    // compound test: when the caller sets a `Vector3` then calling get_int with the same key should return empty
    Material material = generate_material();

    const std::string key = "someKey";
    const float float_value = generate<float>();
    const Vector3 vector3_value = generate<Vector3>();

    material.set<float>(key, float_value);

    ASSERT_TRUE(material.get<float>(key));

    material.set<Vector3>(key, vector3_value);

    ASSERT_TRUE(material.get<Vector3>(key));
    ASSERT_FALSE(material.get<float>(key));
}
