#include "SceneRenderer.hpp"

#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/MaterialPropertyBlock.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneDecorationFlags.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Graphics/TextureGen.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/Config.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/Perf.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "src/Platform/Log.hpp"
#include "src/Utils/Algorithms.hpp"

namespace
{
    osc::Transform GetFloorTransform(glm::vec3 floorLocation, float fixupScaleFactor)
    {
        osc::Transform rv;
        rv.rotation = glm::angleAxis(-osc::fpi2, glm::vec3{1.0f, 0.0f, 0.0f});
        rv.scale = {100.0f * fixupScaleFactor, 100.0f * fixupScaleFactor, 1.0f};
        rv.position = floorLocation;
        return rv;
    }

    osc::AABB WorldpaceAABB(osc::SceneDecoration const& d)
    {
        return osc::TransformAABB(d.mesh->getBounds(), d.transform);
    }

    struct RimHighlights final {
        RimHighlights(
            std::shared_ptr<osc::Mesh const> const& mesh_,
            glm::mat4 const& transform_,
            osc::Material const& material_) :

            mesh{mesh_},
            transform{transform_},
            material{material_}
        {
        }

        std::shared_ptr<osc::Mesh const> mesh;
        glm::mat4 transform;
        osc::Material material;
    };
}


class osc::SceneRenderer::Impl final {
public:
    Impl(Config const& config, MeshCache& meshCache, ShaderCache& shaderCache) :
        m_SceneColoredElementsMaterial{shaderCache.load(config.getResourceDir() / "shaders/SceneShader.vert", config.getResourceDir() / "shaders/SceneShader.frag")},
        m_SceneTexturedElementsMaterial{shaderCache.load(config.getResourceDir() / "shaders/SceneTexturedShader.vert", config.getResourceDir() / "shaders/SceneTexturedShader.frag")},
        m_SolidColorMaterial{shaderCache.load(config.getResourceDir() / "shaders/SceneSolidColor.vert", config.getResourceDir() / "shaders/SceneSolidColor.frag")},
        m_EdgeDetectorMaterial{shaderCache.load(config.getResourceDir() / "shaders/SceneEdgeDetector.vert", config.getResourceDir() / "shaders/SceneEdgeDetector.frag")},
        m_NormalsMaterial{shaderCache.load(config.getResourceDir() / "shaders/SceneNormalsShader.vert", config.getResourceDir() / "shaders/SceneNormalsShader.geom", config.getResourceDir() / "shaders/SceneNormalsShader.frag")},
        m_QuadMesh{meshCache.getTexturedQuadMesh()},
        m_ChequerTexture{GenChequeredFloorTexture()},
        m_Camera{},
        m_RimThickness{1.0f, 1.0f},
        m_MaybeRenderTexture{RenderTextureDescriptor{glm::ivec2{1, 1}}}
    {
        m_SceneTexturedElementsMaterial.setTexture("uDiffuseTexture", m_ChequerTexture);
        m_SceneTexturedElementsMaterial.setVec2("uTextureScale", {200.0f, 200.0f});
        m_RimsSelectedColor.setVec4("uDiffuseColor", {1.0f, 0.0f, 0.0f, 1.0f});
        m_RimsHoveredColor.setVec4("uDiffuseColor", {0.5, 0.0f, 0.0f, 1.0f});
        m_EdgeDetectorMaterial.setTransparent(true);
        m_EdgeDetectorMaterial.setDepthTested(false);
        m_SceneTexturedElementsMaterial.setTransparent(true);
    }

    glm::ivec2 getDimensions() const
    {
        return m_MaybeRenderTexture->getDimensions();
    }

    int32_t getSamples() const
    {
        return m_MaybeRenderTexture->getAntialiasingLevel();
    }

    void draw(nonstd::span<SceneDecoration const> decorations, SceneRendererParams const& params)
    {
        // configure output texture and rims texture to match parameters
        {
            RenderTextureDescriptor desc{params.dimensions};
            desc.setAntialiasingLevel(params.samples);
            EmplaceOrReformat(m_MaybeRenderTexture, desc);
        }

        // update the camera from the input params
        m_Camera.setPosition(params.viewPos);
        m_Camera.setNearClippingPlane(params.nearClippingPlane);
        m_Camera.setFarClippingPlane(params.farClippingPlane);
        m_Camera.setViewMatrixOverride(params.viewMatrix);
        m_Camera.setProjectionMatrixOverride(params.projectionMatrix);

        std::optional<RimHighlights> maybeRimHighlights;

        if (params.drawRims)
        {
            maybeRimHighlights = generateRimHighlights(decorations, params);
        }

        // draw the the scene
        {
            m_SceneColoredElementsMaterial.setVec3("uViewPos", m_Camera.getPosition());
            m_SceneColoredElementsMaterial.setVec3("uLightDir", params.lightDirection);
            m_SceneColoredElementsMaterial.setVec3("uLightColor", params.lightColor);
            m_SceneColoredElementsMaterial.setFloat("uAmbientStrength", params.ambientStrength);
            m_SceneColoredElementsMaterial.setFloat("uDiffuseStrength", params.diffuseStrength);
            m_SceneColoredElementsMaterial.setFloat("uSpecularStrength", params.specularStrength);
            m_SceneColoredElementsMaterial.setFloat("uShininess", params.shininess);
            m_SceneColoredElementsMaterial.setFloat("uNear", m_Camera.getNearClippingPlane());
            m_SceneColoredElementsMaterial.setFloat("uFar", m_Camera.getFarClippingPlane());

            Material transparentMaterial = m_SceneColoredElementsMaterial;
            transparentMaterial.setTransparent(true);

            MaterialPropertyBlock propBlock;
            glm::vec4 lastColor = {-1.0f, -1.0f, -1.0f, 0.0f};
            for (SceneDecoration const& dec : decorations)
            {
                if (dec.color != lastColor)
                {
                    propBlock.setVec4("uDiffuseColor", dec.color);
                    lastColor = dec.color;
                }

                if (dec.maybeMaterial)
                {
                    Graphics::DrawMesh(*dec.mesh, dec.transform, *dec.maybeMaterial, m_Camera, dec.maybeMaterialProps);
                }
                else if (dec.color.a > 0.99f)
                {
                    Graphics::DrawMesh(*dec.mesh, dec.transform, m_SceneColoredElementsMaterial, m_Camera, propBlock);
                }
                else
                {
                    Graphics::DrawMesh(*dec.mesh, dec.transform, transparentMaterial, m_Camera, propBlock);
                }

                // if normals are requested, render the scene element via a normals geometry shader
                if (params.drawMeshNormals)
                {
                    Graphics::DrawMesh(*dec.mesh, dec.transform, m_NormalsMaterial, m_Camera);
                }
            }

            // if a floor is requested, draw a textured floor
            if (params.drawFloor)
            {
                m_SceneTexturedElementsMaterial.setVec3("uViewPos", m_Camera.getPosition());
                m_SceneTexturedElementsMaterial.setVec3("uLightDir", params.lightDirection);
                m_SceneTexturedElementsMaterial.setVec3("uLightColor", params.lightColor);
                m_SceneTexturedElementsMaterial.setFloat("uNear", m_Camera.getNearClippingPlane());
                m_SceneTexturedElementsMaterial.setFloat("uFar", m_Camera.getFarClippingPlane());

                Transform const t = GetFloorTransform(params.floorLocation, params.fixupScaleFactor);

                Graphics::DrawMesh(*m_QuadMesh, t, m_SceneTexturedElementsMaterial, m_Camera);
            }
        }

        // add the rim highlights over the top of the scene texture
        if (maybeRimHighlights)
        {
            Graphics::DrawMesh(*maybeRimHighlights->mesh, maybeRimHighlights->transform, maybeRimHighlights->material, m_Camera);
        }

        // write the scene render to the ouptut texture
        OSC_ASSERT(m_MaybeRenderTexture.has_value());
        m_Camera.setBackgroundColor(params.backgroundColor);
        m_Camera.renderTo(*m_MaybeRenderTexture);
        m_EdgeDetectorMaterial.clearRenderTexture("uScreenTexture");  // prevents copies on next frame
    }

    RenderTexture& updRenderTexture()
    {
        return m_MaybeRenderTexture.value();
    }

private:
    std::optional<RimHighlights> generateRimHighlights(nonstd::span<SceneDecoration const> decorations, SceneRendererParams const& params)
    {
        // compute the worldspace bounds union of all rim-highlighted geometry
        std::optional<AABB> maybeRimWorldspaceAABB;
        for (SceneDecoration const& dec : decorations)
        {
            if (dec.flags & (SceneDecorationFlags_IsSelected | SceneDecorationFlags_IsChildOfSelected | SceneDecorationFlags_IsHovered | SceneDecorationFlags_IsChildOfHovered))
            {
                AABB const decAABB = WorldpaceAABB(dec);
                maybeRimWorldspaceAABB = maybeRimWorldspaceAABB ? Union(*maybeRimWorldspaceAABB, decAABB) : decAABB;
            }
        }

        if (!maybeRimWorldspaceAABB)
        {
            // the scene does not contain any rim-highlighted geometry
            return std::nullopt;
        }

        // figure out if the rims actually appear on the screen and (roughly) where
        std::optional<Rect> maybeRimRectNDC = AABBToScreenNDCRect(
            *maybeRimWorldspaceAABB,
            params.viewMatrix,
            params.projectionMatrix,
            m_Camera.getNearClippingPlane(),
            m_Camera.getFarClippingPlane()
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
        glm::vec2 const rimThicknessNDC = 2.0f*m_RimThickness / glm::vec2{params.dimensions};

        // expand by the rim thickness, so that the output has space for the rims
        rimRectNDC = osc::Expand(rimRectNDC, rimThicknessNDC);

        // constrain the result of the above to within clip space
        rimRectNDC = osc::Clamp(rimRectNDC, {-1.0f, -1.0f}, {1.0f, 1.0f});

        if (osc::Area(rimRectNDC) <= 0.0f)
        {
            // the scene contains rim-highlighted geometry, but it isn't on-screen
            return std::nullopt;
        }

        // compute rim rectangle in texture coordinates
        Rect const rimRectUV = NdcRectToScreenspaceViewportRect(rimRectNDC, Rect{{}, {1.0f, 1.0f}});

        // compute where the quad needs to eventually be drawn in the scene
        Transform quadMeshToRimsQuad;
        quadMeshToRimsQuad.position = {osc::Midpoint(rimRectNDC), 0.0f};
        quadMeshToRimsQuad.scale = {0.5f * osc::Dimensions(rimRectNDC), 1.0f};

        // rendering:

        // draw all selected geometry in a solid color
        for (SceneDecoration const& dec : decorations)
        {
            if (dec.flags & (SceneDecorationFlags_IsSelected | SceneDecorationFlags_IsChildOfSelected))
            {
                Graphics::DrawMesh(*dec.mesh, dec.transform, m_SolidColorMaterial, m_Camera, m_RimsSelectedColor);
            }
            else if (dec.flags & (SceneDecorationFlags_IsHovered | SceneDecorationFlags_IsChildOfHovered))
            {
                Graphics::DrawMesh(*dec.mesh, dec.transform, m_SolidColorMaterial, m_Camera, m_RimsHoveredColor);
            }
        }

        // configure the off-screen solid-colored texture
        RenderTextureDescriptor desc{params.dimensions};
        desc.setAntialiasingLevel(params.samples);
        desc.setColorFormat(RenderTextureFormat::ARGB32);  // care: don't use RED: causes an explosion on some Intel machines (#418)
        EmplaceOrReformat(m_MaybeRimsTexture, desc);
        OSC_ASSERT(m_MaybeRimsTexture.has_value());

        // render to the off-screen solid-colored texture
        glm::vec4 const originalBgColor = m_Camera.getBackgroundColor();
        m_Camera.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
        m_Camera.renderTo(*m_MaybeRimsTexture);
        m_Camera.setBackgroundColor(originalBgColor);

        // configure a material that draws the off-screen colored texture on-screen
        //
        // the off-screen texture is rendered as a quad via an edge-detection kernel
        // that transforms the solid shapes into "rims"
        m_EdgeDetectorMaterial.setRenderTexture("uScreenTexture", *m_MaybeRimsTexture);
        m_EdgeDetectorMaterial.setVec4("uRimRgba", params.rimColor);
        m_EdgeDetectorMaterial.setVec2("uRimThickness", 0.5f*rimThicknessNDC);
        m_EdgeDetectorMaterial.setVec2("uTextureOffset", rimRectUV.p1);
        m_EdgeDetectorMaterial.setVec2("uTextureScale", osc::Dimensions(rimRectUV));

        // return necessary information for rendering the rims
        return RimHighlights
        {
            m_QuadMesh,
            glm::inverse(params.projectionMatrix) * ToMat4(quadMeshToRimsQuad),
            m_EdgeDetectorMaterial
        };
    }

    Material m_SceneColoredElementsMaterial;
    Material m_SceneTexturedElementsMaterial;
    Material m_SolidColorMaterial;
    Material m_EdgeDetectorMaterial;
    Material m_NormalsMaterial;

    std::shared_ptr<Mesh const> m_QuadMesh;
    Texture2D m_ChequerTexture;
    Camera m_Camera;

    glm::vec2 m_RimThickness;
    MaterialPropertyBlock m_RimsSelectedColor;
    MaterialPropertyBlock m_RimsHoveredColor;
    std::optional<RenderTexture> m_MaybeRimsTexture;

    // outputs
    std::optional<RenderTexture> m_MaybeRenderTexture;
};


// public API (PIMPL)

osc::SceneRenderer::SceneRenderer(Config const& config, MeshCache& meshCache, ShaderCache& shaderCache) :
    m_Impl{std::make_unique<Impl>(config, meshCache, shaderCache)}
{
}

osc::SceneRenderer::SceneRenderer(SceneRenderer const& other) :
    m_Impl{std::make_unique<Impl>(*other.m_Impl)}
{
}

osc::SceneRenderer::SceneRenderer(SceneRenderer&&) noexcept = default;
osc::SceneRenderer& osc::SceneRenderer::operator=(SceneRenderer&&) noexcept = default;
osc::SceneRenderer::~SceneRenderer() noexcept = default;

glm::ivec2 osc::SceneRenderer::getDimensions() const
{
    return m_Impl->getDimensions();
}

int32_t osc::SceneRenderer::getSamples() const
{
    return m_Impl->getSamples();
}

void osc::SceneRenderer::draw(nonstd::span<SceneDecoration const> decs, SceneRendererParams const& params)
{
    m_Impl->draw(std::move(decs), params);
}

osc::RenderTexture& osc::SceneRenderer::updRenderTexture()
{
    return m_Impl->updRenderTexture();
}
