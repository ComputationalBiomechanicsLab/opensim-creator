#include "Graphics.h"

#include <liboscar/Graphics/Camera.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/MeshTopology.h>
#include <liboscar/Graphics/Shader.h>
#include <liboscar/Graphics/SubMeshDescriptor.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Platform/App.h>

#include <gtest/gtest.h>

#include <memory>
#include <optional>

using namespace osc;

namespace
{
    std::unique_ptr<App> g_renderer_app;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    class Graphics : public ::testing::Test {
    protected:
        static void SetUpTestSuite()
        {
            g_renderer_app = std::make_unique<App>();
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
}

TEST_F(Graphics, graphics_draw_does_not_throw_with_standard_args)
{
    const Mesh mesh;
    const Transform transform = identity<Transform>();
    const Material material{Shader{c_vertex_shader_src, c_fragment_shader_src}};
    Camera camera;

    ASSERT_NO_THROW({ graphics::draw(mesh, transform, material, camera); });
}

TEST_F(Graphics, graphics_draw_throws_if_given_out_of_bounds_sub_mesh_index)
{
    const Mesh mesh;
    const Transform transform = identity<Transform>();
    const Material material{Shader{c_vertex_shader_src, c_fragment_shader_src}};
    Camera camera;

    ASSERT_ANY_THROW({ graphics::draw(mesh, transform, material, camera, std::nullopt, 0); });
}

TEST_F(Graphics, graphics_draw_does_not_throw_if_given_in_bounds_sub_mesh_index)
{
    Mesh mesh;
    mesh.push_submesh_descriptor({0, 0, MeshTopology::Triangles});
    const Transform transform = identity<Transform>();
    const Material material{Shader{c_vertex_shader_src, c_fragment_shader_src}};
    Camera camera;

    ASSERT_NO_THROW({ graphics::draw(mesh, transform, material, camera, std::nullopt, 0); });
}
