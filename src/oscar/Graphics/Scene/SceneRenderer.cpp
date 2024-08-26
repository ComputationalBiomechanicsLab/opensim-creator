#include "SceneRenderer.h"

#include <oscar/Graphics/Materials/MeshBasicMaterial.h>
#include <oscar/Graphics/Scene/SceneHelpers.h>
#include <oscar/Graphics/Textures/ChequeredTexture.h>
#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/DepthRenderBufferParams.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/RenderTarget.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/PolarPerspectiveCamera.h>
#include <oscar/Maths/QuaternionFunctions.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Transform.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Utils/Perf.h>
#include <oscar/Utils/StdVariantHelpers.h>

#include <algorithm>
#include <memory>
#include <span>
#include <utility>

using namespace osc::literals;
using namespace osc;

namespace
{
    const StringName c_diffuse_color_propname{"uDiffuseColor"};

    Transform calc_floor_transform(Vec3 floor_origin, float fixup_scale_factor)
    {
        return {
            .scale = {100.0f * fixup_scale_factor, 100.0f * fixup_scale_factor, 1.0f},
            .rotation = angle_axis(-90_deg, Vec3{1.0f, 0.0f, 0.0f}),
            .position = floor_origin,
        };
    }

    struct RimHighlights final {

        RimHighlights(
            Mesh mesh_,
            const Mat4& transform_,
            Material material_) :

            mesh{std::move(mesh_)},
            transform{transform_},
            material{std::move(material_)}
        {}

        Mesh mesh;
        Mat4 transform;
        Material material;
    };

    struct Shadows final {
        SharedDepthRenderBuffer shadow_map;
        Mat4 lightspace_mat;
    };

    struct PolarAngles final {
        Radians theta;
        Radians phi;
    };

    PolarAngles calc_polar_angles(const Vec3& direction_from_origin)
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

    struct ShadowCameraMatrices final {
        Mat4 view_mat;
        Mat4 projection_mat;
    };

    ShadowCameraMatrices calc_shadow_camera_matrices(
        const AABB& shadowcasters_aabb,
        const Vec3& light_direction)
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
        camera.znear = 0.0f;
        camera.zfar = 2.0f * shadowcasters_sphere.radius;

        const Mat4 view_mat = camera.view_matrix();
        const Mat4 projection_mat = ortho(
            -shadowcasters_sphere.radius,
            shadowcasters_sphere.radius,
            -shadowcasters_sphere.radius,
            shadowcasters_sphere.radius,
            0.0f,
            2.0f*shadowcasters_sphere.radius
        );

        return ShadowCameraMatrices{view_mat, projection_mat};
    }

    // the `Material` that's used to shade the main scene (colored `SceneDecoration`s)
    class SceneMainMaterial final : public Material {
    public:
        explicit SceneMainMaterial(SceneCache& cache) :
            Material{cache.get_shader(
                "oscar/shaders/SceneRenderer/DrawColoredObjects.vert",
                "oscar/shaders/SceneRenderer/DrawColoredObjects.frag"
            )}
        {}
    };

    // the `Material` that's used to shade the scene's floor (special case)
    class SceneFloorMaterial final : public Material {
    public:
        explicit SceneFloorMaterial(SceneCache& cache) :
            Material{cache.get_shader(
                "oscar/shaders/SceneRenderer/DrawTexturedObjects.vert",
                "oscar/shaders/SceneRenderer/DrawTexturedObjects.frag"
            )}
        {
            set<Texture2D>("uDiffuseTexture", ChequeredTexture{});
            set("uTextureScale", Vec2{200.0f, 200.0f});
            set_transparent(true);
        }
    };

    // the `Material` that's used to detect the edges, per channel, in the input texture (used for rim-highlighting)
    class EdgeDetectionMaterial final : public Material {
    public:
        explicit EdgeDetectionMaterial(SceneCache& cache) :
            Material{cache.get_shader(
                "oscar/shaders/SceneRenderer/EdgeDetector.vert",
                "oscar/shaders/SceneRenderer/EdgeDetector.frag"
            )}
        {
            set_transparent(true);    // so that anti-aliased edged alpha-blend correctly
            set_depth_tested(false);  // not required: it's handling a single quad
        }
    };

    // the `Material` that's used to draw mesh surface normal vectors
    class NormalsMaterial final : public Material {
    public:
        explicit NormalsMaterial(SceneCache& cache) :
            Material{cache.get_shader(
                "oscar/shaders/SceneRenderer/NormalsVisualizer.vert",
                "oscar/shaders/SceneRenderer/NormalsVisualizer.geom",
                "oscar/shaders/SceneRenderer/NormalsVisualizer.frag"
            )}
        {}
    };

    // a `Material` that emits the NDC depth of the fragment as a color
    class DepthColoringMaterial final : public Material {
    public:
        explicit DepthColoringMaterial(SceneCache& cache) :
            Material{cache.get_shader(
                "oscar/shaders/SceneRenderer/DepthMap.vert",
                "oscar/shaders/SceneRenderer/DepthMap.frag"
            )}
        {}
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

        scene_main_material_{cache},
        scene_floor_material_{cache},
        rim_filler_material_{cache},
        wireframe_material_{cache.wireframe_material()},
        edge_detection_material_{cache},
        normals_material_{cache},
        depth_writer_material_{cache},
        quad_mesh_{cache.quad_mesh()}
    {
        wireframe_material_.set_color(Color::black());
    }

    void render(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& params)
    {
        // render any other perspectives on the scene (shadows, rim highlights, etc.)
        const std::optional<RimHighlights> maybe_rims = try_generate_rims(decorations, params);
        const std::optional<Shadows> maybe_shadowmap = try_generate_shadowmap(decorations, params);

        // setup camera for this render
        camera_.reset();
        camera_.set_position(params.view_pos);
        camera_.set_clipping_planes({params.near_clipping_plane, params.far_clipping_plane});
        camera_.set_view_matrix_override(params.view_matrix);
        camera_.set_projection_matrix_override(params.projection_matrix);
        camera_.set_background_color(params.background_color);

        // draw the the scene
        {
            scene_main_material_.set("uViewPos", camera_.position());
            scene_main_material_.set("uLightDir", params.light_direction);
            scene_main_material_.set("uLightColor", params.light_color);
            scene_main_material_.set("uAmbientStrength", params.ambient_strength);
            scene_main_material_.set("uDiffuseStrength", params.diffuse_strength);
            scene_main_material_.set("uSpecularStrength", params.specular_strength);
            scene_main_material_.set("uShininess", params.specular_shininess);
            scene_main_material_.set("uNear", camera_.near_clipping_plane());
            scene_main_material_.set("uFar", camera_.far_clipping_plane());

            // supply shadowmap, if applicable
            if (maybe_shadowmap) {
                scene_main_material_.set("uHasShadowMap", true);
                scene_main_material_.set("uLightSpaceMat", maybe_shadowmap->lightspace_mat);
                scene_main_material_.set("uShadowMapTexture", maybe_shadowmap->shadow_map);
            }
            else {
                scene_main_material_.set("uHasShadowMap", false);
            }

            Material transparent_material = scene_main_material_;
            transparent_material.set_transparent(true);

            MaterialPropertyBlock prop_block;
            MaterialPropertyBlock wireframe_prop_block;
            Color previous_color = {-1.0f, -1.0f, -1.0f, 0.0f};
            for (const SceneDecoration& dec : decorations) {
                if (dec.flags & SceneDecorationFlag::NoDrawInScene) {
                    continue;  // skip this
                }

                Color color_guess = Color::white();
                std::visit(Overload{
                    [this, &transparent_material, &dec, &previous_color, &prop_block, &color_guess](const Color& color)
                    {
                        if (color != previous_color) {
                            prop_block.set(c_diffuse_color_propname, color);
                            previous_color = color;
                        }

                        if (color.a > 0.99f) {
                            graphics::draw(dec.mesh, dec.transform, scene_main_material_, camera_, prop_block);
                        }
                        else {
                            graphics::draw(dec.mesh, dec.transform, transparent_material, camera_, prop_block);
                        }
                        color_guess = color;
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

                // if a wireframe overlay is requested for the decoration then draw it over the top in
                // a solid color
                if (dec.flags & SceneDecorationFlag::DrawWireframeOverlay) {
                    wireframe_prop_block.set(c_diffuse_color_propname, multiply_luminance(color_guess, 0.1f));
                    graphics::draw(dec.mesh, dec.transform, wireframe_material_, camera_, wireframe_prop_block);
                }

                // if normals are requested, render the scene element via a normals geometry shader
                //
                // care: this only works for triangles, because normals-drawing material uses a geometry
                //       shader that assumes triangular input (#792)
                if (params.draw_mesh_normals and dec.mesh.topology() == MeshTopology::Triangles) {
                    graphics::draw(dec.mesh, dec.transform, normals_material_, camera_);
                }
            }

            // if a floor is requested, draw a textured floor
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

                // supply shadowmap, if applicable
                if (maybe_shadowmap) {
                    scene_floor_material_.set("uHasShadowMap", true);
                    scene_floor_material_.set("uLightSpaceMat", maybe_shadowmap->lightspace_mat);
                    scene_floor_material_.set("uShadowMapTexture", maybe_shadowmap->shadow_map);
                }
                else {
                    scene_floor_material_.set("uHasShadowMap", false);
                }

                const Transform t = calc_floor_transform(params.floor_location, params.fixup_scale_factor);

                graphics::draw(quad_mesh_, t, scene_floor_material_, camera_);
            }
        }

        // add the rim highlights over the top of the scene texture
        if (maybe_rims) {
            graphics::draw(maybe_rims->mesh, maybe_rims->transform, maybe_rims->material, camera_);
        }

        output_rendertexture_.set_dimensions(params.dimensions);
        output_rendertexture_.set_anti_aliasing_level(params.antialiasing_level);
        camera_.render_to(output_rendertexture_);

        // prevents copies on next frame
        edge_detection_material_.unset("uScreenTexture");
        scene_floor_material_.unset("uShadowMapTexture");
        scene_main_material_.unset("uShadowMapTexture");
    }

    RenderTexture& upd_render_texture()
    {
        return output_rendertexture_;
    }

private:
    std::optional<RimHighlights> try_generate_rims(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& params)
    {
        if (not params.draw_rims) {
            return std::nullopt;
        }

        // compute the worldspace bounds union of all rim-highlighted geometry
        const auto rim_aabb_of = [](const SceneDecoration& decoration) -> std::optional<AABB>
        {
            if (decoration.is_rim_highlighted()) {
                return worldspace_bounds_of(decoration);
            }
            return std::nullopt;
        };

        const std::optional<AABB> maybe_rim_worldspace_aabb = maybe_bounding_aabb_of(decorations, rim_aabb_of);
        if (not maybe_rim_worldspace_aabb) {
            return std::nullopt;  // the scene does not contain any rim-highlighted geometry
        }

        // figure out if the rims actually appear on the screen and (roughly) where
        std::optional<Rect> maybe_rim_ndc_rect = loosely_project_into_ndc(
            *maybe_rim_worldspace_aabb,
            params.view_matrix,
            params.projection_matrix,
            params.near_clipping_plane,
            params.far_clipping_plane
        );
        if (not maybe_rim_ndc_rect) {
            return std::nullopt;  // the scene contains rim-highlighted geometry, but it isn't on-screen
        }
        // else: the scene contains rim-highlighted geometry that may appear on screen

        // the rims appear on the screen and are loosely bounded (in NDC) by the returned rect
        Rect& rim_ndc_rect = *maybe_rim_ndc_rect;

        // compute rim thickness in each direction (aspect ratio might not be 1:1)
        const Vec2 rim_ndc_thickness = 2.0f*params.rim_thickness_in_pixels / Vec2{params.dimensions};

        // expand by the rim thickness, so that the output has space for the rims
        rim_ndc_rect = expand_by_absolute_amount(rim_ndc_rect, rim_ndc_thickness);

        // constrain the result of the above to within clip space
        rim_ndc_rect = clamp(rim_ndc_rect, {-1.0f, -1.0f}, {1.0f, 1.0f});

        if (area_of(rim_ndc_rect) <= 0.0f) {
            return std::nullopt;  // the scene contains rim-highlighted geometry, but it isn't on-screen
        }

        // compute rim rectangle in texture coordinates
        const Rect rim_rect_uv = ndc_rect_to_screenspace_viewport_rect(rim_ndc_rect, Rect{{}, {1.0f, 1.0f}});

        // compute where the quad needs to eventually be drawn in the scene
        Transform quad_mesh_to_rims_quad{
            .scale = {0.5f * dimensions_of(rim_ndc_rect), 1.0f},
            .position = {centroid_of(rim_ndc_rect), 0.0f},
        };

        // rendering:

        // setup scene camera
        camera_.reset();
        camera_.set_position(params.view_pos);
        camera_.set_clipping_planes({params.near_clipping_plane, params.far_clipping_plane});
        camera_.set_view_matrix_override(params.view_matrix);
        camera_.set_projection_matrix_override(params.projection_matrix);
        camera_.set_background_color(Color::clear());

        // draw all selected geometry in a solid color
        std::unordered_map<Color, MeshBasicMaterial::PropertyBlock> block_cache;
        block_cache.reserve(3);  // guess
        for (const SceneDecoration& decoration : decorations) {
            Color color = Color::black();

            static_assert(SceneRendererParams::num_rim_groups() == 2);
            if (decoration.flags & SceneDecorationFlag::RimHighlight0) {
                color.r = 1.0f;
            }
            if (decoration.flags & SceneDecorationFlag::RimHighlight1) {
                color.g = 1.0f;
            }

            if (color != Color::black()) {
                const auto& prop_block = block_cache.try_emplace(color, color).first->second;
                graphics::draw(decoration.mesh, decoration.transform, rim_filler_material_, camera_, prop_block);
            }
        }

        // configure the off-screen solid-colored texture
        rims_rendertexture_.reformat({
            .dimensions = params.dimensions,
            .anti_aliasing_level = params.antialiasing_level,
            .color_format = RenderTextureFormat::ARGB32,
        });

        // render to the off-screen solid-colored texture
        camera_.render_to(RenderTarget{
            RenderTargetColorAttachment{
                .color_buffer = rims_rendertexture_.upd_color_buffer(),
            },
        });

        // configure a material that draws the off-screen colored texture on-screen
        //
        // the off-screen texture is rendered as a quad via an edge-detection kernel
        // that transforms the solid shapes into "rims"
        edge_detection_material_.set("uScreenTexture", rims_rendertexture_.upd_color_buffer());
        static_assert(SceneRendererParams::num_rim_groups() == 2);
        edge_detection_material_.set("uRim0Color", params.rim_group_colors[0]);
        edge_detection_material_.set("uRim1Color", params.rim_group_colors[1]);
        edge_detection_material_.set("uRimThickness", 0.5f*rim_ndc_thickness);
        edge_detection_material_.set("uTextureOffset", rim_rect_uv.p1);
        edge_detection_material_.set("uTextureScale", dimensions_of(rim_rect_uv));

        // return necessary information for rendering the rims
        return RimHighlights{
            quad_mesh_,
            inverse(params.projection_matrix * params.view_matrix) * mat4_cast(quad_mesh_to_rims_quad),
            edge_detection_material_,
        };
    }

    std::optional<Shadows> try_generate_shadowmap(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& params)
    {
        if (not params.draw_shadows) {
            return std::nullopt;  // the caller doesn't actually want shadows
        }

        // setup scene camera
        camera_.reset();

        // compute the bounds of everything that casts a shadow
        //
        // (also, while doing that, draw each mesh - to prevent multipass)
        std::optional<AABB> shadowcaster_aabbs;
        for (const SceneDecoration& decoration : decorations) {
            if (decoration.flags & SceneDecorationFlag::NoCastsShadows) {
                continue;  // this decoration shouldn't cast shadows
            }
            shadowcaster_aabbs = bounding_aabb_of(shadowcaster_aabbs, worldspace_bounds_of(decoration));
            graphics::draw(decoration.mesh, decoration.transform, depth_writer_material_, camera_);
        }

        if (not shadowcaster_aabbs) {
            // there are no shadow casters, so there will be no shadows
            camera_.reset();
            return std::nullopt;
        }

        // compute camera matrices for the orthogonal (direction) camera used for lighting
        const ShadowCameraMatrices matrices = calc_shadow_camera_matrices(*shadowcaster_aabbs, params.light_direction);

        camera_.set_view_matrix_override(matrices.view_mat);
        camera_.set_projection_matrix_override(matrices.projection_mat);
        camera_.render_to(RenderTarget{
            RenderTargetDepthAttachment{
                .buffer = shadowmap_render_buffer_,
            },
        });

        return Shadows{shadowmap_render_buffer_ , matrices.projection_mat * matrices.view_mat};
    }

    SceneMainMaterial scene_main_material_;
    SceneFloorMaterial scene_floor_material_;
    RimFillerMaterial rim_filler_material_;
    MeshBasicMaterial wireframe_material_;
    EdgeDetectionMaterial edge_detection_material_;
    NormalsMaterial normals_material_;
    DepthColoringMaterial depth_writer_material_;

    Mesh quad_mesh_;
    Camera camera_;
    RenderTexture rims_rendertexture_;
    SharedDepthRenderBuffer shadowmap_render_buffer_{{
        .dimensions = {1024, 1024},
    }};
    RenderTexture output_rendertexture_;
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
