#include "SceneRenderer.h"

#include <oscar/Graphics/Materials/MeshBasicMaterial.h>
#include <oscar/Graphics/Textures/ChequeredTexture.h>
#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/RenderTexture.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneDecorationFlags.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Graphics/Scene/ShaderCache.h>
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

#include <algorithm>
#include <memory>
#include <span>
#include <utility>

using namespace osc::literals;
using namespace osc;

namespace
{
    Transform GetFloorTransform(Vec3 floorLocation, float fixupScaleFactor)
    {
        return {
            .scale = {100.0f * fixupScaleFactor, 100.0f * fixupScaleFactor, 1.0f},
            .rotation = angle_axis(-90_deg, Vec3{1.0f, 0.0f, 0.0f}),
            .position = floorLocation,
        };
    }

    AABB WorldpaceAABB(SceneDecoration const& d)
    {
        return transform_aabb(d.mesh.getBounds(), d.transform);
    }

    struct RimHighlights final {

        RimHighlights(
            Mesh mesh_,
            Mat4 const& transform_,
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
        RenderTexture shadowMap;
        Mat4 lightSpaceMat;
    };

    struct PolarAngles final {
        Radians theta;
        Radians phi;
    };

    PolarAngles CalcPolarAngles(Vec3 const& directionFromOrigin)
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
            .theta = osc::atan2(directionFromOrigin.x, directionFromOrigin.z),
            .phi = osc::asin(directionFromOrigin.y),
        };
    }

    struct ShadowCameraMatrices final {
        Mat4 viewMatrix;
        Mat4 projMatrix;
    };

    ShadowCameraMatrices CalcShadowCameraMatrices(
        AABB const& casterAABBs,
        Vec3 const& lightDirection)
    {
        Sphere const casterSphere = ToSphere(casterAABBs);
        PolarAngles const cameraPolarAngles = CalcPolarAngles(-lightDirection);

        // pump sphere+polar information into a polar camera in order to
        // calculate the renderer's view/projection matrices
        PolarPerspectiveCamera polarCamera;
        polarCamera.focusPoint = -casterSphere.origin;
        polarCamera.phi = cameraPolarAngles.phi;
        polarCamera.theta = cameraPolarAngles.theta;
        polarCamera.radius = casterSphere.radius;
        polarCamera.znear = 0.0f;
        polarCamera.zfar = 2.0f * casterSphere.radius;

        Mat4 const viewMat = polarCamera.view_matrix();
        Mat4 const projMat = ortho(
            -casterSphere.radius,
            casterSphere.radius,
            -casterSphere.radius,
            casterSphere.radius,
            0.0f,
            2.0f*casterSphere.radius
        );

        return ShadowCameraMatrices{viewMat, projMat};
    }
}


class osc::SceneRenderer::Impl final {
public:
    Impl(SceneCache& meshCache, ShaderCache& shaderCache) :

        m_SceneColoredElementsMaterial{shaderCache.load("oscar/shaders/SceneRenderer/DrawColoredObjects.vert", "oscar/shaders/SceneRenderer/DrawColoredObjects.frag")},
        m_SceneTexturedElementsMaterial{shaderCache.load("oscar/shaders/SceneRenderer/DrawTexturedObjects.vert", "oscar/shaders/SceneRenderer/DrawTexturedObjects.frag")},
        m_SolidColorMaterial{shaderCache.basic_material()},
        m_WireframeMaterial{shaderCache.wireframe_material()},
        m_EdgeDetectorMaterial{shaderCache.load("oscar/shaders/SceneRenderer/EdgeDetector.vert", "oscar/shaders/SceneRenderer/EdgeDetector.frag")},
        m_NormalsMaterial{shaderCache.load("oscar/shaders/SceneRenderer/NormalsVisualizer.vert", "oscar/shaders/SceneRenderer/NormalsVisualizer.geom", "oscar/shaders/SceneRenderer/NormalsVisualizer.frag")},
        m_DepthWritingMaterial{shaderCache.load("oscar/shaders/SceneRenderer/DepthMap.vert", "oscar/shaders/SceneRenderer/DepthMap.frag")},
        m_QuadMesh{meshCache.getTexturedQuadMesh()}
    {
        m_SceneTexturedElementsMaterial.setTexture("uDiffuseTexture", m_ChequerTexture);
        m_SceneTexturedElementsMaterial.setVec2("uTextureScale", {200.0f, 200.0f});
        m_SceneTexturedElementsMaterial.setTransparent(true);

        m_WireframeMaterial.setColor(Color::black());

        m_RimsSelectedColor.setColor(Color::red());
        m_RimsHoveredColor.setColor({0.5, 0.0f, 0.0f, 1.0f});

        m_EdgeDetectorMaterial.setTransparent(true);
        m_EdgeDetectorMaterial.setDepthTested(false);
    }

    Vec2i getDimensions() const
    {
        return m_OutputTexture.getDimensions();
    }

    AntiAliasingLevel getAntiAliasingLevel() const
    {
        return m_OutputTexture.getAntialiasingLevel();
    }

    void render(
        std::span<SceneDecoration const> decorations,
        SceneRendererParams const& params)
    {
        // render any other perspectives on the scene (shadows, rim highlights, etc.)
        std::optional<RimHighlights> const maybeRimHighlights = tryGenerateRimHighlights(decorations, params);
        std::optional<Shadows> const maybeShadowMap = tryGenerateShadowMap(decorations, params);

        // setup camera for this render
        m_Camera.reset();
        m_Camera.setPosition(params.viewPos);
        m_Camera.setNearClippingPlane(params.nearClippingPlane);
        m_Camera.setFarClippingPlane(params.farClippingPlane);
        m_Camera.setViewMatrixOverride(params.viewMatrix);
        m_Camera.setProjectionMatrixOverride(params.projectionMatrix);
        m_Camera.setBackgroundColor(params.backgroundColor);

        // draw the the scene
        {
            m_SceneColoredElementsMaterial.setVec3("uViewPos", m_Camera.getPosition());
            m_SceneColoredElementsMaterial.setVec3("uLightDir", params.lightDirection);
            m_SceneColoredElementsMaterial.setColor("uLightColor", params.lightColor);
            m_SceneColoredElementsMaterial.setFloat("uAmbientStrength", params.ambientStrength);
            m_SceneColoredElementsMaterial.setFloat("uDiffuseStrength", params.diffuseStrength);
            m_SceneColoredElementsMaterial.setFloat("uSpecularStrength", params.specularStrength);
            m_SceneColoredElementsMaterial.setFloat("uShininess", params.specularShininess);
            m_SceneColoredElementsMaterial.setFloat("uNear", m_Camera.getNearClippingPlane());
            m_SceneColoredElementsMaterial.setFloat("uFar", m_Camera.getFarClippingPlane());

            // supply shadowmap, if applicable
            if (maybeShadowMap)
            {
                m_SceneColoredElementsMaterial.setBool("uHasShadowMap", true);
                m_SceneColoredElementsMaterial.setMat4("uLightSpaceMat", maybeShadowMap->lightSpaceMat);
                m_SceneColoredElementsMaterial.setRenderTexture("uShadowMapTexture", maybeShadowMap->shadowMap);
            }
            else
            {
                m_SceneColoredElementsMaterial.setBool("uHasShadowMap", false);
            }

            Material transparentMaterial = m_SceneColoredElementsMaterial;
            transparentMaterial.setTransparent(true);

            MaterialPropertyBlock propBlock;
            MaterialPropertyBlock wireframePropBlock;
            Color lastColor = {-1.0f, -1.0f, -1.0f, 0.0f};
            for (SceneDecoration const& dec : decorations)
            {
                if (!(dec.flags & SceneDecorationFlags::NoDrawNormally)) {
                    if (dec.color != lastColor) {
                        propBlock.setColor("uDiffuseColor", dec.color);
                        lastColor = dec.color;
                    }

                    if (dec.maybeMaterial) {
                        Graphics::DrawMesh(dec.mesh, dec.transform, *dec.maybeMaterial, m_Camera, dec.maybeMaterialProps);
                    }
                    else if (dec.color.a > 0.99f) {
                        Graphics::DrawMesh(dec.mesh, dec.transform, m_SceneColoredElementsMaterial, m_Camera, propBlock);
                    }
                    else {
                        Graphics::DrawMesh(dec.mesh, dec.transform, transparentMaterial, m_Camera, propBlock);
                    }
                }

                // if a wireframe overlay is requested for the decoration then draw it over the top in
                // a solid color
                if (dec.flags & SceneDecorationFlags::WireframeOverlay) {
                    wireframePropBlock.setColor("uDiffuseColor",MultiplyLuminance(dec.color, 0.5f));
                    Graphics::DrawMesh(dec.mesh, dec.transform, m_WireframeMaterial, m_Camera, wireframePropBlock);
                }

                // if normals are requested, render the scene element via a normals geometry shader
                //
                // care: this only works for triangles, because normals-drawing material uses a geometry
                //       shader that assumes triangular input (#792)
                if (params.drawMeshNormals && dec.mesh.getTopology() == MeshTopology::Triangles) {
                    Graphics::DrawMesh(dec.mesh, dec.transform, m_NormalsMaterial, m_Camera);
                }
            }

            // if a floor is requested, draw a textured floor
            if (params.drawFloor)
            {
                m_SceneTexturedElementsMaterial.setVec3("uViewPos", m_Camera.getPosition());
                m_SceneTexturedElementsMaterial.setVec3("uLightDir", params.lightDirection);
                m_SceneTexturedElementsMaterial.setColor("uLightColor", params.lightColor);
                m_SceneTexturedElementsMaterial.setFloat("uAmbientStrength", 0.7f);
                m_SceneTexturedElementsMaterial.setFloat("uDiffuseStrength", 0.4f);
                m_SceneTexturedElementsMaterial.setFloat("uSpecularStrength", 0.4f);
                m_SceneTexturedElementsMaterial.setFloat("uShininess", 8.0f);
                m_SceneTexturedElementsMaterial.setFloat("uNear", m_Camera.getNearClippingPlane());
                m_SceneTexturedElementsMaterial.setFloat("uFar", m_Camera.getFarClippingPlane());

                // supply shadowmap, if applicable
                if (maybeShadowMap)
                {
                    m_SceneTexturedElementsMaterial.setBool("uHasShadowMap", true);
                    m_SceneTexturedElementsMaterial.setMat4("uLightSpaceMat", maybeShadowMap->lightSpaceMat);
                    m_SceneTexturedElementsMaterial.setRenderTexture("uShadowMapTexture", maybeShadowMap->shadowMap);
                }
                else
                {
                    m_SceneTexturedElementsMaterial.setBool("uHasShadowMap", false);
                }

                Transform const t = GetFloorTransform(params.floorLocation, params.fixupScaleFactor);

                Graphics::DrawMesh(m_QuadMesh, t, m_SceneTexturedElementsMaterial, m_Camera);
            }
        }

        // add the rim highlights over the top of the scene texture
        if (maybeRimHighlights)
        {
            Graphics::DrawMesh(maybeRimHighlights->mesh, maybeRimHighlights->transform, maybeRimHighlights->material, m_Camera);
        }

        m_OutputTexture.setDimensions(params.dimensions);
        m_OutputTexture.setAntialiasingLevel(params.antiAliasingLevel);
        m_Camera.renderTo(m_OutputTexture);

        // prevents copies on next frame
        m_EdgeDetectorMaterial.clearRenderTexture("uScreenTexture");
        m_SceneTexturedElementsMaterial.clearRenderTexture("uShadowMapTexture");
        m_SceneColoredElementsMaterial.clearRenderTexture("uShadowMapTexture");
    }

    RenderTexture& updRenderTexture()
    {
        return m_OutputTexture;
    }

private:
    std::optional<RimHighlights> tryGenerateRimHighlights(
        std::span<SceneDecoration const> decorations,
        SceneRendererParams const& params)
    {
        if (!params.drawRims)
        {
            return std::nullopt;
        }

        // compute the worldspace bounds union of all rim-highlighted geometry
        auto const rimAABB = [](SceneDecoration const& d) -> std::optional<AABB>
        {
            if (d.flags & (SceneDecorationFlags::IsSelected | SceneDecorationFlags::IsChildOfSelected | SceneDecorationFlags::IsHovered | SceneDecorationFlags::IsChildOfHovered)) {
                return WorldpaceAABB(d);
            }
            return std::nullopt;
        };
        std::optional<AABB> maybeRimWorldspaceAABB = maybe_aabb_of(decorations, rimAABB);

        if (!maybeRimWorldspaceAABB)
        {
            // the scene does not contain any rim-highlighted geometry
            return std::nullopt;
        }

        // figure out if the rims actually appear on the screen and (roughly) where
        std::optional<Rect> maybeRimRectNDC = loosely_project_into_ndc(
            *maybeRimWorldspaceAABB,
            params.viewMatrix,
            params.projectionMatrix,
            params.nearClippingPlane,
            params.farClippingPlane
        );

        if (!maybeRimRectNDC)
        {
            // the scene contains rim-highlighted geometry, but it isn't on-screen
            return std::nullopt;
        }
        // else: the scene contains rim-highlighted geometry that may appear on screen

        // the rims appear on the screen and are loosely bounded (in NDC) by the returned rect
        Rect& rimRectNDC = *maybeRimRectNDC;

        // compute rim thickness in each direction (aspect ratio might not be 1:1)
        Vec2 const rimThicknessNDC = 2.0f*params.rimThicknessInPixels / Vec2{params.dimensions};

        // expand by the rim thickness, so that the output has space for the rims
        rimRectNDC = Expand(rimRectNDC, rimThicknessNDC);

        // constrain the result of the above to within clip space
        rimRectNDC = Clamp(rimRectNDC, {-1.0f, -1.0f}, {1.0f, 1.0f});

        if (Area(rimRectNDC) <= 0.0f)
        {
            // the scene contains rim-highlighted geometry, but it isn't on-screen
            return std::nullopt;
        }

        // compute rim rectangle in texture coordinates
        Rect const rimRectUV = NdcRectToScreenspaceViewportRect(rimRectNDC, Rect{{}, {1.0f, 1.0f}});

        // compute where the quad needs to eventually be drawn in the scene
        Transform quadMeshToRimsQuad;
        quadMeshToRimsQuad.position = {centroid(rimRectNDC), 0.0f};
        quadMeshToRimsQuad.scale = {0.5f * dimensions(rimRectNDC), 1.0f};

        // rendering:

        // setup scene camera
        m_Camera.reset();
        m_Camera.setPosition(params.viewPos);
        m_Camera.setNearClippingPlane(params.nearClippingPlane);
        m_Camera.setFarClippingPlane(params.farClippingPlane);
        m_Camera.setViewMatrixOverride(params.viewMatrix);
        m_Camera.setProjectionMatrixOverride(params.projectionMatrix);
        m_Camera.setBackgroundColor(Color::clear());

        // draw all selected geometry in a solid color
        for (SceneDecoration const& dec : decorations)
        {
            if (dec.flags & (SceneDecorationFlags::IsSelected | SceneDecorationFlags::IsChildOfSelected))
            {
                Graphics::DrawMesh(dec.mesh, dec.transform, m_SolidColorMaterial, m_Camera, m_RimsSelectedColor);
            }
            else if (dec.flags & (SceneDecorationFlags::IsHovered | SceneDecorationFlags::IsChildOfHovered))
            {
                Graphics::DrawMesh(dec.mesh, dec.transform, m_SolidColorMaterial, m_Camera, m_RimsHoveredColor);
            }
        }

        // configure the off-screen solid-colored texture
        RenderTextureDescriptor desc{params.dimensions};
        desc.setAntialiasingLevel(params.antiAliasingLevel);
        desc.setColorFormat(RenderTextureFormat::ARGB32);  // care: don't use RED: causes an explosion on some Intel machines (#418)
        m_RimsTexture.reformat(desc);

        // render to the off-screen solid-colored texture
        m_Camera.renderTo(m_RimsTexture);

        // configure a material that draws the off-screen colored texture on-screen
        //
        // the off-screen texture is rendered as a quad via an edge-detection kernel
        // that transforms the solid shapes into "rims"
        m_EdgeDetectorMaterial.setRenderTexture("uScreenTexture", m_RimsTexture);
        m_EdgeDetectorMaterial.setColor("uRimRgba", params.rimColor);
        m_EdgeDetectorMaterial.setVec2("uRimThickness", 0.5f*rimThicknessNDC);
        m_EdgeDetectorMaterial.setVec2("uTextureOffset", rimRectUV.p1);
        m_EdgeDetectorMaterial.setVec2("uTextureScale", dimensions(rimRectUV));

        // return necessary information for rendering the rims
        return RimHighlights
        {
            m_QuadMesh,
            inverse(params.projectionMatrix * params.viewMatrix) * mat4_cast(quadMeshToRimsQuad),
            m_EdgeDetectorMaterial,
        };
    }

    std::optional<Shadows> tryGenerateShadowMap(
        std::span<SceneDecoration const> decorations,
        SceneRendererParams const& params)
    {
        if (!params.drawShadows) {
            return std::nullopt;  // the caller doesn't actually want shadows
        }

        // setup scene camera
        m_Camera.reset();

        // compute the bounds of everything that casts a shadow
        //
        // (also, while doing that, draw each mesh - to prevent multipass)
        std::optional<AABB> casterAABBs;
        for (SceneDecoration const& dec : decorations) {
            if (dec.flags & SceneDecorationFlags::CastsShadows) {
                casterAABBs = aabb_of(casterAABBs, WorldpaceAABB(dec));
                Graphics::DrawMesh(dec.mesh, dec.transform, m_DepthWritingMaterial, m_Camera);
            }
        }

        if (!casterAABBs) {
            // there are no shadow casters, so there will be no shadows
            m_Camera.reset();
            return std::nullopt;
        }

        // compute camera matrices for the orthogonal (direction) camera used for lighting
        ShadowCameraMatrices const matrices = CalcShadowCameraMatrices(*casterAABBs, params.lightDirection);

        m_Camera.setBackgroundColor({1.0f, 0.0f, 0.0f, 0.0f});
        m_Camera.setViewMatrixOverride(matrices.viewMatrix);
        m_Camera.setProjectionMatrixOverride(matrices.projMatrix);
        m_ShadowMapTexture.setDimensions({1024, 1024});
        m_ShadowMapTexture.setReadWrite(RenderTextureReadWrite::Linear);  // it's writing distances
        m_Camera.renderTo(m_ShadowMapTexture);

        return Shadows{m_ShadowMapTexture, matrices.projMatrix * matrices.viewMatrix};
    }

    Material m_SceneColoredElementsMaterial;
    Material m_SceneTexturedElementsMaterial;
    MeshBasicMaterial m_SolidColorMaterial;
    MeshBasicMaterial m_WireframeMaterial;
    Material m_EdgeDetectorMaterial;
    Material m_NormalsMaterial;
    Material m_DepthWritingMaterial;
    MeshBasicMaterial::PropertyBlock m_RimsSelectedColor;
    MeshBasicMaterial::PropertyBlock m_RimsHoveredColor;
    Mesh m_QuadMesh;
    Texture2D m_ChequerTexture = ChequeredTexture{};
    Camera m_Camera;
    RenderTexture m_RimsTexture;
    RenderTexture m_ShadowMapTexture;
    RenderTexture m_OutputTexture;
};


// public API (PIMPL)

osc::SceneRenderer::SceneRenderer(SceneCache& meshCache, ShaderCache& shaderCache) :
    m_Impl{std::make_unique<Impl>(meshCache, shaderCache)}
{
}

osc::SceneRenderer::SceneRenderer(SceneRenderer const& other) :
    m_Impl{std::make_unique<Impl>(*other.m_Impl)}
{
}

osc::SceneRenderer::SceneRenderer(SceneRenderer&&) noexcept = default;
osc::SceneRenderer& osc::SceneRenderer::operator=(SceneRenderer&&) noexcept = default;
osc::SceneRenderer::~SceneRenderer() noexcept = default;

Vec2i osc::SceneRenderer::getDimensions() const
{
    return m_Impl->getDimensions();
}

AntiAliasingLevel osc::SceneRenderer::getAntiAliasingLevel() const
{
    return m_Impl->getAntiAliasingLevel();
}

void osc::SceneRenderer::render(
    std::span<SceneDecoration const> decs,
    SceneRendererParams const& params)
{
    m_Impl->render(decs, params);
}

RenderTexture& osc::SceneRenderer::updRenderTexture()
{
    return m_Impl->updRenderTexture();
}
