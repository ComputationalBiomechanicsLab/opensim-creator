#include "SceneRenderer.h"

#include <liboscar/Graphics/Camera.h>
#include <liboscar/Graphics/CameraClearFlags.h>
#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/ColorRenderBufferParams.h>
#include <liboscar/Graphics/DepthStencilRenderBufferParams.h>
#include <liboscar/Graphics/Graphics.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/MaterialPropertyBlock.h>
#include <liboscar/Graphics/Materials/MeshBasicMaterial.h>
#include <liboscar/Graphics/Materials/MeshDepthWritingMaterial.h>
#include <liboscar/Graphics/Materials/MeshNormalVectorsMaterial.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/RenderTarget.h>
#include <liboscar/Graphics/RenderTexture.h>
#include <liboscar/Graphics/Scene/SceneCache.h>
#include <liboscar/Graphics/Scene/SceneDecoration.h>
#include <liboscar/Graphics/Scene/SceneDecorationFlags.h>
#include <liboscar/Graphics/Scene/SceneRendererParams.h>
#include <liboscar/Graphics/Textures/ChequeredTexture.h>
#include <liboscar/Maths/AABB.h>
#include <liboscar/Maths/AABBFunctions.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/CoordinateDirection.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/Matrix4x4.h>
#include <liboscar/Maths/MatrixFunctions.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/QuaternionFunctions.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Sphere.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/StdVariantHelpers.h>
#include <liboscar/Utils/StringName.h>

#include <ankerl/unordered_dense.h>

#include <algorithm>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

using namespace osc::literals;
using namespace osc;

namespace
{
    const StringName c_diffuse_color_propname{"uDiffuseColor"};

    struct RimHighlights final {
        Mesh mesh;
        Matrix4x4 transform;
        Material material;
    };

    struct Shadows final {
        SharedDepthStencilRenderBuffer shadow_map;
        Matrix4x4 lightspace_matrix;
    };

    struct PolarAngles final {
        Radians theta;
        Radians phi;
    };

    struct ShadowCameraMatrices final {
        Matrix4x4 view_matrix;
        Matrix4x4 projection_matrix;
    };

    Transform calc_floor_transform(Vector3 floor_origin, float fixup_scale_factor)
    {
        // note: this should be the same as `osc::draw_grid`
        return {
            .scale = {50.0f * fixup_scale_factor, 50.0f * fixup_scale_factor, 1.0f},
            .rotation = angle_axis(-90_deg, CoordinateDirection::x()),
            .translation = floor_origin,
        };
    }

    PolarAngles calc_polar_angles(const Vector3& direction_from_origin)
    {
        // X is left-to-right
        // Y is bottom-to-top
        // Z is near-to-far
        //
        // combinations:
        //
        // | theta |   phi  | X  | Y  | Z  |
        // | ----- | ------ | -- | -- | -- |
        // |     0 |      0 |  0 |  0 | 1  |
        // |  pi/2 |      0 |  1 |  0 |  0 |
        // |     0 |   pi/2 |  0 |  1 |  0 |

        return {
            .theta = osc::atan2(direction_from_origin.x, direction_from_origin.z),
            .phi = osc::asin(direction_from_origin.y),
        };
    }

    ShadowCameraMatrices calc_shadow_camera_matrices(
        const AABB& shadowcasters_aabb,
        const Vector3& light_direction)
    {
        const Sphere shadowcasters_sphere = bounding_sphere_of(shadowcasters_aabb);
        const PolarAngles camera_polar_angles = calc_polar_angles(-light_direction);

        // pump sphere+polar information into a polar camera in order to
        // calculate the renderer's view/projection matrices
        PolarPerspectiveCamera camera;
        camera.focus_point = -shadowcasters_sphere.origin;
        camera.phi = camera_polar_angles.phi;
        camera.theta = camera_polar_angles.theta;
        camera.radius = shadowcasters_sphere.radius;

        const Matrix4x4 view_matrix = camera.view_matrix();
        const Matrix4x4 projection_matrix = ortho(
            -shadowcasters_sphere.radius,
            shadowcasters_sphere.radius,
            -shadowcasters_sphere.radius,
            shadowcasters_sphere.radius,
            0.0f,
            2.0f*shadowcasters_sphere.radius
        );

        return ShadowCameraMatrices{view_matrix, projection_matrix};
    }

    // compute the world space bounds union of all rim-highlighted geometry
    std::optional<AABB> rim_aabb_of(const SceneDecoration& decoration)
    {
        return decoration.has_flag(SceneDecorationFlag::AllRimHighlightGroups) ?
            std::optional(decoration.world_space_bounds()) :
            std::nullopt;
    }

    // the `Material` that's used to shade the main scene (colored `SceneDecoration`s)
    class SceneMainMaterial final : public Material {
    public:
        explicit SceneMainMaterial() :
            Material{Shader{c_vertex_shader_src, c_fragment_shader_src}}
        {}

    private:
        static constexpr std::string_view c_vertex_shader_src = R"(
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

                // care: these lighting calculations use "double-sided normals", because
                // mesh data from users can have screwed normals/winding, but OSC still
                // should try its best to render it "correct enough" (#168, #318)
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

        static constexpr std::string_view c_fragment_shader_src = R"(
            #version 330 core

            uniform bool uHasShadowMap = false;
            uniform bool uIsOITPass = false;
            uniform vec3 uLightDir;
            uniform sampler2D uShadowMapTexture;
            uniform float uAmbientStrength = 0.15f;
            uniform vec4 uLightColor;
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
                if (!uHasShadowMap) {
                    return 0.0;
                }

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
                for (int x = -1; x <= 1; ++x) {
                    for (int y = -1; y <= 1; ++y) {
                        float pcfDepth = texture(uShadowMapTexture, projCoords.xy + vec2(x, y) * texelSize).r;
                        if (pcfDepth < 1.0) {
                            shadow += (currentDepth - bias) > pcfDepth  ? 1.0 : 0.0;
                        }
                    }
                }
                shadow /= 9.0;

                return 0.5*shadow;
            }

            void main()
            {
                float brightness = uAmbientStrength + (NonAmbientBrightness * (1.0 - CalculateShadowAmount()));
                vec4 fragColor = vec4(brightness * vec3(uLightColor), 1.0) * uDiffuseColor;

                if (uIsOITPass) {
                    float weight = fragColor.a; // simple
                    // float weight = fragColor.a * (1.0 - 0.5 * gl_FragCoord.z); // some attenuation
                    // float weight = clamp(pow(fragColor.a + 0.01, 4.0) * 1e3 * pow(1.0 - gl_FragCoord.z, 3.0), 1e-2, 3e3);  // published
                    Color0Out = vec4(fragColor.rgb * fragColor.a * weight, fragColor.a * weight);  // OIT accumulator
                } else {
                    Color0Out = fragColor;
                }
            }
        )";
    };

    // A material that composites OIT output into a scene overlay
    class SceneOITCompositorMaterial final : public Material {
    public:
        explicit SceneOITCompositorMaterial() :
            Material{Shader{c_vertex_shader_src, c_fragment_shader_src}}
        {}
    private:
        static constexpr std::string_view c_vertex_shader_src = R"(
            #version 330 core

            uniform mat4 uModelMat;
            uniform mat4 uViewProjMat;
            uniform vec2 uTextureOffset = vec2(0.0, 0.0);
            uniform vec2 uTextureScale = vec2(1.0, 1.0);

            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;

            out vec2 TexCoords;

            void main()
            {
                TexCoords = uTextureOffset + (uTextureScale * aTexCoord);
                gl_Position = uViewProjMat * uModelMat * vec4(aPos, 1.0);
            }
        )";
        static constexpr std::string_view c_fragment_shader_src = R"(
            #version 330 core

            uniform sampler2D uOITAccumulator;

            in vec2 TexCoords;
            out vec4 FragColor;

            void main()
            {
                vec4 sample = texture(uOITAccumulator, TexCoords);
                FragColor = vec4(sample.rgb / max(sample.a, 1e-5), clamp(sample.a, 0.0, 1.0));
            }
        )";
    };

    // the `Material` that's used to shade the scene's floor (special case)
    class SceneFloorMaterial final : public Material {
    public:
        SceneFloorMaterial() :
            Material{Shader{c_vertex_shader_src, c_fragment_shader_src}}
        {
            set<Texture2D>("uDiffuseTexture", ChequeredTexture{});
            set("uTextureScale", Vector2{100.0f});
            set_transparent(true);
        }

    private:
        static constexpr std::string_view c_vertex_shader_src = R"(
            #version 330 core

            uniform mat4 uViewProjMat;
            uniform mat4 uLightSpaceMat;
            uniform vec3 uLightDir;
            uniform vec3 uViewPos;
            uniform vec2 uTextureScale = vec2(1.0, 1.0);
            uniform float uDiffuseStrength;
            uniform float uSpecularStrength;
            uniform float uShininess;

            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            layout (location = 2) in vec3 aNormal;
            layout (location = 6) in mat4 aModelMat;
            layout (location = 10) in mat3 aNormalMat;

            out vec3 FragWorldPos;
            out vec4 FragLightSpacePos;
            out vec3 NormalWorldDir;
            out float NonAmbientBrightness;
            out vec2 TexCoord;

            void main()
            {
                vec3 normalDir = normalize(aNormalMat * aNormal);
                vec3 fragPos = vec3(aModelMat * vec4(aPos, 1.0));
                vec3 frag2viewDir = normalize(uViewPos - fragPos);
                vec3 frag2lightDir = normalize(-uLightDir);
                vec3 halfwayDir = 0.5 * (frag2lightDir + frag2viewDir);

                // care: these lighting calculations use "double-sided normals", because
                // mesh data from users can have screwed normals/winding, but OSC still
                // should try its best to render it "correct enough" (#168, #318)
                float diffuseAmt = uDiffuseStrength * abs(dot(normalDir, frag2lightDir));
                float specularAmt = uSpecularStrength * pow(abs(dot(normalDir, halfwayDir)), uShininess);

                vec4 worldPos = aModelMat * vec4(aPos, 1.0);

                FragWorldPos = vec3(worldPos);
                FragLightSpacePos = uLightSpaceMat * vec4(FragWorldPos, 1.0);
                NormalWorldDir = normalDir;
                NonAmbientBrightness = diffuseAmt + specularAmt;
                TexCoord = uTextureScale * aTexCoord;

                gl_Position = uViewProjMat * worldPos;
            }
        )";

        static constexpr std::string_view c_fragment_shader_src = R"(
            #version 330 core

            uniform bool uHasShadowMap = false;
            uniform sampler2D uDiffuseTexture;
            uniform vec3 uLightDir;
            uniform sampler2D uShadowMapTexture;
            uniform float uAmbientStrength;
            uniform vec4 uLightColor;
            uniform float uNear;
            uniform float uFar;

            in vec3 FragWorldPos;
            in vec4 FragLightSpacePos;
            in vec3 NormalWorldDir;
            in float NonAmbientBrightness;
            in vec2 TexCoord;

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
                        float pcfDepth = texture(uShadowMapTexture, projCoords.xy + vec2(x, y)*texelSize).r;
                        if (pcfDepth < 1.0)
                        {
                            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
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
                float shadowAmt = uHasShadowMap ? 0.5*CalculateShadowAmount() : 0.0;
                float brightness = clamp(uAmbientStrength + ((1.0 - shadowAmt) * NonAmbientBrightness), 0.0, 1.0);
                Color0Out = brightness * vec4(brightness * vec3(uLightColor), 1.0) * texture(uDiffuseTexture, TexCoord);
                Color0Out.a *= 1.0 - (LinearizeDepth(gl_FragCoord.z) / uFar);  // fade into background at high distances
                Color0Out.a = clamp(Color0Out.a, 0.0, 1.0);
            }
        )";
    };

    // the `Material` that's used to detect the edges, per color component, in the input texture (used for rim-highlighting)
    class EdgeDetectionMaterial final : public Material {
    public:
        explicit EdgeDetectionMaterial() :
            Material{Shader{c_vertex_shader_src, c_fragment_shader_src}}
        {
            set_transparent(true);    // so that anti-aliased edges alpha-blend correctly
            set_depth_tested(false);  // not required: it's handling a single quad
        }

    private:
        static constexpr std::string_view c_vertex_shader_src = R"(
            #version 330 core

            uniform mat4 uModelMat;
            uniform mat4 uViewProjMat;
            uniform vec2 uTextureOffset = vec2(0.0, 0.0);
            uniform vec2 uTextureScale = vec2(1.0, 1.0);

            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;

            out vec2 TexCoords;

            void main()
            {
                TexCoords = uTextureOffset + (uTextureScale * aTexCoord);
                gl_Position = uViewProjMat * uModelMat * vec4(aPos, 1.0);
            }
        )";

        static constexpr std::string_view c_fragment_shader_src = R"(
            #version 330 core

            uniform sampler2D uScreenTexture;
            uniform vec4 uRim0Color;
            uniform vec4 uRim1Color;
            uniform vec2 uRimThickness;

            in vec2 TexCoords;
            out vec4 FragColor;

            // sampling offsets to use when retrieving samples to feed
            // into the kernel
            const vec2 g_TextureOffsets[9] = vec2[](
                vec2(-1.0,  1.0), // top-left
                vec2( 0.0,  1.0), // top-center
                vec2( 1.0,  1.0), // top-right
                vec2(-1.0,  0.0), // center-left
                vec2( 0.0,  0.0), // center-center
                vec2( 1.0,  0.0), // center-right
                vec2(-1.0, -1.0), // bottom-left
                vec2( 0.0, -1.0), // bottom-center
                vec2( 1.0, -1.0)  // bottom-right
            );

            // https://computergraphics.stackexchange.com/questions/2450/opengl-detection-of-edges
            //
            // this is known as a "Sobel Kernel"
            const vec2 g_KernelCoefficients[9] = vec2[](
                vec2( 1.0,  1.0),  // top-left
                vec2( 0.0,  2.0),  // top-center
                vec2(-1.0,  1.0),  // top-right

                vec2( 2.0,  0.0),  // center-left
                vec2( 0.0,  0.0),  // center
                vec2(-2.0,  0.0),  // center-right

                vec2( 1.0, -1.0),  // bottom-left
                vec2( 0.0, -2.0),  // bottom-center
                vec2(-1.0, -1.0)   // bottom-right
            );

            void main(void)
            {
                vec2 rim0XY = vec2(0.0, 0.0);
                vec2 rim1XY = vec2(0.0, 0.0);
                for (int i = 0; i < g_KernelCoefficients.length(); ++i) {
                    vec2 offset = uRimThickness * g_TextureOffsets[i];
                    vec2 coord = TexCoords + offset;
                    vec2 v = texture(uScreenTexture, coord).rg;
                    rim0XY += v.r * g_KernelCoefficients[i];
                    rim1XY += v.g * g_KernelCoefficients[i];
                }

                // the maximum value from the Sobel Kernel is sqrt(3^2 + 3^2) == sqrt(18)
                //
                // but lowering the scaling factor a bit is handy for making the rims more solid
                float rim0Strength = length(rim0XY) / 4.242640;
                float rim1Strength = length(rim1XY) / 4.242640;

                vec4 rim0Color = rim0Strength * uRim0Color;
                vec4 rim1Color = rim1Strength * uRim1Color;

                FragColor = rim0Color + rim1Color;
            }
        )";
    };

    // a `Material` that colors `SceneDecoration`s in the rim color (groups)
    class RimFillerMaterial final : public MeshBasicMaterial {
    public:
        explicit RimFillerMaterial(SceneCache& cache) :
            MeshBasicMaterial{cache.basic_material()}
        {
            set_depth_tested(false);
            set_transparent(true);
            set_source_blending_factor(SourceBlendingFactor::One);
            set_destination_blending_factor(DestinationBlendingFactor::One);
            set_blending_equation(BlendingEquation::Max);
        }
    };
}

class osc::SceneRenderer::Impl final {
public:
    explicit Impl(SceneCache& cache) :

        scene_main_material_{cache.get<SceneMainMaterial>()},
        scene_floor_material_{cache.get<SceneFloorMaterial>()},
        scene_oit_compositor_material_{cache.get<SceneOITCompositorMaterial>()},
        rim_filler_material_{cache},
        wireframe_material_{cache.wireframe_material()},
        edge_detection_material_{cache.get<EdgeDetectionMaterial>()},
        quad_mesh_{cache.quad_mesh()}
    {
        wireframe_material_.set_color(Color::black());
        backface_culled_rim_filler_material_.set_cull_mode(CullMode::Back);
        backface_culled_depth_writer_material_.set_cull_mode(CullMode::Back);
    }

    void render(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& params)
    {
        const std::optional<RimHighlights> maybe_rims = try_generate_rims(decorations, params);
        const std::optional<Shadows> maybe_shadow_map = try_generate_shadow_map(decorations, params);

        // Setup camera (parameters are the same for all scene render passes)
        camera_.reset();
        camera_.set_position(params.viewer_position);
        camera_.set_clipping_planes({params.near_clipping_plane, params.far_clipping_plane});
        camera_.set_view_matrix_override(params.view_matrix);
        camera_.set_projection_matrix_override(params.projection_matrix);
        camera_.set_background_color(params.background_color);
        camera_.set_clear_flags(CameraClearFlag::Default);

        // Setup final output texture params (doesn't change during passes)
        output_render_texture_.set_pixel_dimensions(params.device_pixel_ratio * params.dimensions);
        output_render_texture_.set_device_pixel_ratio(params.device_pixel_ratio);
        output_render_texture_.set_anti_aliasing_level(params.anti_aliasing_level);

        render_objects_to_output_render(decorations, params, maybe_shadow_map);
        if (params.order_independent_transparency) {
            // If rendering with order-independent transparency (OIT) is desired, render transparent
            // objects to OIT accumulators and then composite the accumulators using OIT maths over
            // the output render
            if (render_transparent_objects_to_oit_accumulators(decorations, params)) {
                scene_oit_compositor_material_.set("uOITAccumulator", oit_render_buffer_);
                scene_oit_compositor_material_.set_depth_tested(false);
                scene_oit_compositor_material_.set_transparent(true);
                graphics::draw(quad_mesh_, camera_.inverse_view_projection_matrix(aspect_ratio_of(params.dimensions)), scene_oit_compositor_material_, camera_);
                camera_.set_clear_flags(CameraClearFlag::None);
                camera_.render_to(output_render_texture_);
            }
        }

        // Composite rim highlights over the top of the final render
        if (maybe_rims) {
            graphics::draw(maybe_rims->mesh, maybe_rims->transform, maybe_rims->material, camera_);
            camera_.set_clear_flags(CameraClearFlag::None);
            camera_.render_to(output_render_texture_);
        }

        // prevents copies on next frame
        edge_detection_material_.unset("uScreenTexture");
        scene_main_material_.unset("uShadowMapTexture");
        scene_floor_material_.unset("uShadowMapTexture");
        scene_oit_compositor_material_.unset("uOITAccumulator");
    }

    RenderTexture& upd_render_texture()
    {
        return output_render_texture_;
    }

private:
    void render_objects_to_output_render(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& params,
        const std::optional<Shadows>& maybe_shadow_map)
    {
        // Setup opaque object material parameters
        scene_main_material_.set("uViewPos", camera_.position());
        scene_main_material_.set("uLightDir", params.light_direction);
        scene_main_material_.set("uLightColor", params.light_color);
        scene_main_material_.set("uAmbientStrength", params.ambient_strength);
        scene_main_material_.set("uDiffuseStrength", params.diffuse_strength);
        scene_main_material_.set("uSpecularStrength", params.specular_strength);
        scene_main_material_.set("uShininess", params.specular_shininess);
        scene_main_material_.set("uNear", camera_.near_clipping_plane());
        scene_main_material_.set("uFar", camera_.far_clipping_plane());
        scene_main_material_.set("uIsOITPass", false);

        // Supply shadow map (if available)
        if (maybe_shadow_map) {
            scene_main_material_.set("uHasShadowMap", true);
            scene_main_material_.set("uLightSpaceMat", maybe_shadow_map->lightspace_matrix);
            scene_main_material_.set("uShadowMapTexture", maybe_shadow_map->shadow_map);
        }
        else {
            scene_main_material_.set("uHasShadowMap", false);
        }

        const Material& opaque_material{scene_main_material_};
        Material backface_culled_opaque_material{opaque_material};
        backface_culled_opaque_material.set_cull_mode(CullMode::Back);
        Material transparent_material{opaque_material};
        transparent_material.set_transparent(true);
        Material backface_culled_transparent_material{transparent_material};
        backface_culled_transparent_material.set_cull_mode(CullMode::Back);

        color_cache_.clear();
        MaterialPropertyBlock wireframe_prop_block;

        // draw scene decorations
        for (const SceneDecoration& dec : decorations) {

            // if a wireframe overlay is requested for the decoration then draw it over the top in
            // a solid color - even if `NoDrawInScene` is requested (#952).
            if (dec.has_flag(SceneDecorationFlag::DrawWireframeOverlay)) {
                const Color wireframe_color = std::visit(Overload{
                    [](const Color& color) { return color; },
                    [](const auto&) { return Color::white(); },
                }, dec.shading);

                wireframe_prop_block.set(c_diffuse_color_propname, multiply_luminance(wireframe_color, 0.25f));
                graphics::draw(dec.mesh, dec.transform, wireframe_material_, camera_, wireframe_prop_block);
            }

            if (dec.has_flag(SceneDecorationFlag::NoDrawInScene)) {
                continue;  // skip drawing the decoration (and, potentially, its normals)
            }

            std::visit(Overload{
                [this, &params, &opaque_material, &backface_culled_opaque_material, &transparent_material, &backface_culled_transparent_material, &dec](const Color& color)
                {
                    const Color32 color32{color};  // Renderer doesn't need HDR colors
                    if (color32.a != 0xff and params.order_independent_transparency) {
                        return;  // OIT is handled in a seperate pass
                    }

                    const auto& [it, inserted] = color_cache_.try_emplace(color32);
                    if (inserted) {
                        it->second.set(c_diffuse_color_propname, color32);
                    }
                    const bool backface_culled = dec.has_flag(SceneDecorationFlag::CanBackfaceCull);
                    const Material& material = color32.a == 0xff ?
                            (backface_culled ? backface_culled_opaque_material : opaque_material) :
                            (backface_culled ? backface_culled_transparent_material : transparent_material);
                    graphics::draw(dec.mesh, dec.transform, material, camera_, it->second);
                },
                [this, &dec](const Material& material)
                {
                    graphics::draw(dec.mesh, dec.transform, material, camera_);
                },
                [this, &dec](const std::pair<Material, MaterialPropertyBlock>& material_props_pair)
                {
                    graphics::draw(dec.mesh, dec.transform, material_props_pair.first, camera_, material_props_pair.second);
                }
            }, dec.shading);

            // if normals are requested, render the scene element via a normals geometry shader
            //
            // care: this only works for triangles, because normals-drawing material uses a geometry
            //       shader that assumes triangular input (#792)
            if (params.draw_mesh_normals and dec.mesh.topology() == MeshTopology::Triangles) {
                graphics::draw(dec.mesh, dec.transform, normals_material_, camera_);
            }
        }

        // If a floor is requested, draw an opaque textured floor
        if (params.draw_floor) {
            scene_floor_material_.set("uViewPos", camera_.position());
            scene_floor_material_.set("uLightDir", params.light_direction);
            scene_floor_material_.set("uLightColor", params.light_color);
            scene_floor_material_.set("uAmbientStrength", 0.7f);
            scene_floor_material_.set("uDiffuseStrength", 0.4f);
            scene_floor_material_.set("uSpecularStrength", 0.4f);
            scene_floor_material_.set("uShininess", 8.0f);
            scene_floor_material_.set("uNear", camera_.near_clipping_plane());
            scene_floor_material_.set("uFar", camera_.far_clipping_plane());

            // supply shadow map, if applicable
            if (maybe_shadow_map) {
                scene_floor_material_.set("uHasShadowMap", true);
                scene_floor_material_.set("uLightSpaceMat", maybe_shadow_map->lightspace_matrix);
                scene_floor_material_.set("uShadowMapTexture", maybe_shadow_map->shadow_map);
            }
            else {
                scene_floor_material_.set("uHasShadowMap", false);
            }

            graphics::draw(
                quad_mesh_,
                calc_floor_transform(params.floor_position, params.fixup_scale_factor),
                scene_floor_material_,
                camera_
            );
        }

        camera_.set_clear_flags(CameraClearFlag::Default);
        camera_.render_to(output_render_texture_);
    }

    bool render_transparent_objects_to_oit_accumulators(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& params)
    {
        // Render transparent objects to OIT accumulators (use ADD blending, use opaque depth test but no depth writing).
        SceneMainMaterial oit_material{scene_main_material_};
        oit_material.set_transparent(true);
        oit_material.set_source_blending_factor(SourceBlendingFactor::One);
        oit_material.set_destination_blending_factor(DestinationBlendingFactor::One);
        oit_material.set_blending_equation(BlendingEquation::Add);
        oit_material.set_writes_to_depth_buffer(false);
        oit_material.set("uIsOITPass", true);

        // Draw transparent colored scene elements.
        color_cache_.clear();
        for (const SceneDecoration& decoration : decorations) {

            if (decoration.has_flag(SceneDecorationFlag::NoDrawInScene)) {
                continue;  // Skip drawing the decoration (and, potentially, its normals)
            }

            // Draw opaque/custom decoration
            std::visit(Overload{
                [this, &decoration, &oit_material](const Color& color)
                {
                    const Color32 color32{color};  // Renderer doesn't need HDR colors
                    if (color32.a == 0xff) {
                        return;  // Skip opaque objects (already drawn)
                    }

                    const auto& [it, inserted] = color_cache_.try_emplace(color32);
                    if (inserted) {
                        it->second.set(c_diffuse_color_propname, color32);
                    }
                    graphics::draw(decoration.mesh, decoration.transform, oit_material, camera_, it->second);
                },
                [](const auto&) {},  // Skip custom decorations (already done)
            }, decoration.shading);
        }

        if (color_cache_.empty()) {
            return false;  // No transparent objects in the scene
        }

        // Ensure OIT buffer is correctly formatted
        const Vector2i pixel_dimensions = params.device_pixel_ratio * params.dimensions;
        if (oit_render_buffer_.pixel_dimensions() != pixel_dimensions or
            oit_render_buffer_.anti_aliasing_level() != params.anti_aliasing_level) {

            oit_render_buffer_ = SharedColorRenderBuffer{{
                .pixel_dimensions = pixel_dimensions,
                .dimensionality = TextureDimensionality::Tex2D,
                .anti_aliasing_level = params.anti_aliasing_level,
                .format = ColorRenderBufferFormat::R16G16B16A16_SFLOAT,
            }};
        }

        // Render to OIT floating-point buffer
        camera_.render_to(RenderTarget{
            RenderTargetColorAttachment{
                .buffer = oit_render_buffer_,
                .load_action = RenderBufferLoadAction::Clear,
                .store_action = RenderBufferStoreAction::Resolve,
                .clear_color = Color::clear(),
            },
            RenderTargetDepthStencilAttachment{
                .buffer = output_render_texture_.upd_depth_buffer(),
                .load_action = RenderBufferLoadAction::Load,        // Don't clear opaque depth
                .store_action = RenderBufferStoreAction::DontCare,  // Depth writing is disabled
            }
        });

        return true;
    }

    std::optional<RimHighlights> try_generate_rims(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& params)
    {
        if (not params.draw_rims) {
            return std::nullopt;
        }

        const std::optional<AABB> maybe_rim_world_space_aabb = maybe_bounding_aabb_of(decorations, rim_aabb_of);
        if (not maybe_rim_world_space_aabb) {
            return std::nullopt;  // the scene does not contain any rim-highlighted geometry
        }

        // figure out if the rims actually appear on the screen and (roughly) where
        std::optional<Rect> maybe_rim_ndc_rect = loosely_project_into_ndc(
            *maybe_rim_world_space_aabb,
            params.view_matrix,
            params.projection_matrix,
            params.near_clipping_plane,
            params.far_clipping_plane
        );
        if (not maybe_rim_ndc_rect) {
            return std::nullopt;  // the scene contains rim-highlighted geometry, but it isn't on-screen
        }

        // else: the rims appear on the screen and are loosely bounded (in NDC) by the returned rect
        Rect& rim_ndc_rect = *maybe_rim_ndc_rect;

        // compute rim thickness in each direction (aspect ratio might not be 1:1)
        const Vector2 rim_ndc_thickness = 2.0f * params.rim_thickness/params.dimensions;

        // expand by 2x the rim thickness, so that the output has space on both sides for the rims
        rim_ndc_rect = rim_ndc_rect.with_dimensions(rim_ndc_rect.dimensions() + 2.0f*rim_ndc_thickness);

        // constrain the result to within clip space
        rim_ndc_rect = clamp(rim_ndc_rect, {-1.0f, -1.0f}, {1.0f, 1.0f});

        // compute rim rectangle in texture coordinates
        const Rect rim_rect_uv = ndc_rect_to_topleft_viewport_rect(rim_ndc_rect, Rect::from_corners({}, {1.0f, 1.0f}));

        // compute where the quad needs to eventually be drawn in the scene
        const Transform quad_mesh_to_rims_quad{
            .scale = Vector3{rim_ndc_rect.half_extents(), 1.0f},
            .translation = Vector3{rim_ndc_rect.origin(), 0.0f},
        };

        // setup scene camera
        camera_.reset();
        camera_.set_position(params.viewer_position);
        camera_.set_clipping_planes({params.near_clipping_plane, params.far_clipping_plane});
        camera_.set_view_matrix_override(params.view_matrix);
        camera_.set_projection_matrix_override(params.projection_matrix);
        camera_.set_background_color(Color::clear());

        // draw all selected geometry in a solid color
        color_cache_.clear();
        for (const SceneDecoration& decoration : decorations) {
            Color32 color = Color32::black();

            static_assert(SceneRendererParams::num_rim_groups() == 2);
            if (decoration.has_flag(SceneDecorationFlag::RimHighlight0)) {
                color.r = 1.0f;
            }
            if (decoration.has_flag(SceneDecorationFlag::RimHighlight1)) {
                color.g = 1.0f;
            }

            if (color != Color32::black()) {
                const auto& [it, inserted] = color_cache_.try_emplace(color);
                if (inserted) {
                    it->second.set(MeshBasicMaterial::color_property_name(), color);
                }
                const Material& material = decoration.has_flag(SceneDecorationFlag::CanBackfaceCull) ?
                    backface_culled_rim_filler_material_ :
                    rim_filler_material_;
                graphics::draw(decoration.mesh, decoration.transform, material, camera_, it->second);
            }
        }

        // configure the off-screen solid-colored texture
        rims_render_texture_.reformat({
            .pixel_dimensions = params.device_pixel_ratio * params.dimensions,
            .device_pixel_ratio = params.device_pixel_ratio,
            .anti_aliasing_level = params.anti_aliasing_level,
        });

        // render to the off-screen solid-colored texture
        camera_.render_to(RenderTarget{
            RenderTargetColorAttachment{
                .buffer = rims_render_texture_.upd_color_buffer(),
            },
        });

        // configure a material that draws the off-screen colored texture on-screen
        //
        // the off-screen texture is rendered as a quad via an edge-detection kernel
        // that transforms the solid shapes into "rims"
        static_assert(SceneRendererParams::num_rim_groups() == 2, "Check below if the number of groups changed");
        edge_detection_material_.set("uScreenTexture", rims_render_texture_.upd_color_buffer());
        edge_detection_material_.set("uRim0Color", params.rim_group_colors[0]);
        edge_detection_material_.set("uRim1Color", params.rim_group_colors[1]);
        edge_detection_material_.set("uRimThickness", 0.5f*rim_ndc_thickness);
        edge_detection_material_.set("uTextureOffset", rim_rect_uv.ypu_bottom_left());
        edge_detection_material_.set("uTextureScale", rim_rect_uv.dimensions());

        // return necessary information for rendering the rims
        return RimHighlights{
            .mesh = quad_mesh_,
            .transform = inverse(params.projection_matrix * params.view_matrix) * matrix4x4_cast(quad_mesh_to_rims_quad),
            .material = edge_detection_material_,
        };
    }

    std::optional<Shadows> try_generate_shadow_map(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& params)
    {
        if (not params.draw_shadows) {
            return std::nullopt;
        }

        camera_.reset();

        // compute the bounds of, and draw, everything that casts a shadow
        std::optional<AABB> shadowcaster_aabbs;
        for (const SceneDecoration& decoration : decorations) {
            if (decoration.has_flag(SceneDecorationFlag::NoCastsShadows)) {
                continue;  // this decoration shouldn't cast shadows
            }
            shadowcaster_aabbs = bounding_aabb_of(shadowcaster_aabbs, decoration.world_space_bounds());
            const Material& material = decoration.has_flag(SceneDecorationFlag::CanBackfaceCull) ?
                backface_culled_depth_writer_material_ :
                depth_writer_material_;
            graphics::draw(decoration.mesh, decoration.transform, material, camera_);
        }

        if (not shadowcaster_aabbs) {
            camera_.reset();
            return std::nullopt;  // no shadow casters (therefore, no shadows)
        }

        // compute camera matrices for the orthogonal (direction) camera used for lighting
        const ShadowCameraMatrices matrices = calc_shadow_camera_matrices(*shadowcaster_aabbs, params.light_direction);

        camera_.set_view_matrix_override(matrices.view_matrix);
        camera_.set_projection_matrix_override(matrices.projection_matrix);
        camera_.render_to(RenderTarget{
            RenderTargetDepthStencilAttachment{
                .buffer = shadow_map_render_buffer_,
            },
        });

        return Shadows{
            .shadow_map = shadow_map_render_buffer_ ,
            .lightspace_matrix = matrices.projection_matrix * matrices.view_matrix,
        };
    }

    SceneMainMaterial scene_main_material_;
    SceneFloorMaterial scene_floor_material_;
    SceneOITCompositorMaterial scene_oit_compositor_material_;
    RimFillerMaterial rim_filler_material_;
    RimFillerMaterial backface_culled_rim_filler_material_{rim_filler_material_};
    MeshBasicMaterial wireframe_material_;
    EdgeDetectionMaterial edge_detection_material_;
    MeshNormalVectorsMaterial normals_material_;
    MeshDepthWritingMaterial depth_writer_material_;
    MeshDepthWritingMaterial backface_culled_depth_writer_material_{depth_writer_material_};

    ankerl::unordered_dense::map<Color32, MaterialPropertyBlock> color_cache_;

    Mesh quad_mesh_;
    Camera camera_;
    RenderTexture rims_render_texture_;
    SharedDepthStencilRenderBuffer shadow_map_render_buffer_{DepthStencilRenderBufferParams{
        .pixel_dimensions = {1024, 1024},
    }};
    SharedColorRenderBuffer oit_render_buffer_;
    RenderTexture output_render_texture_;
};

osc::SceneRenderer::SceneRenderer(SceneCache& scene_cache) :
    impl_{std::make_unique<Impl>(scene_cache)}
{}
osc::SceneRenderer::SceneRenderer(const SceneRenderer& other) :
    impl_{std::make_unique<Impl>(*other.impl_)}
{}
osc::SceneRenderer::SceneRenderer(SceneRenderer&&) noexcept = default;
osc::SceneRenderer& osc::SceneRenderer::operator=(SceneRenderer&&) noexcept = default;
osc::SceneRenderer::~SceneRenderer() noexcept = default;

void osc::SceneRenderer::render(
    std::span<const SceneDecoration> decorations,
    const SceneRendererParams& params)
{
    impl_->render(decorations, params);
}

RenderTexture& osc::SceneRenderer::upd_render_texture()
{
    return impl_->upd_render_texture();
}
