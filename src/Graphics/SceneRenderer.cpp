#include "SceneRenderer.hpp"

#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneDecorationFlags.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Graphics/TextureGen.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/Perf.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

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
}


class osc::SceneRenderer::Impl final {
public:
    Impl()
    {
        m_SolidColorMaterial.setTransparent(true);
        m_SceneTexturedElementsMaterial.setTexture("uDiffuseTexture", m_ChequerTexture);
        m_SceneTexturedElementsMaterial.setVec2("uTextureScale", {200.0f, 200.0f});
    }

    glm::ivec2 getDimensions() const
    {
        return {m_MaybeRenderTexture->getWidth(), m_MaybeRenderTexture->getHeight()};
    }

    int getSamples() const
    {
        return m_MaybeRenderTexture->getAntialiasingLevel();
    }

    void draw(nonstd::span<SceneDecoration const> decorations, SceneRendererParams const& params)
    {
        constexpr glm::vec2 rimThickness{1.0f};

        // configure output texture to match requested dimensions/samples
        RenderTextureDescriptor desc
        {
            static_cast<int>(params.dimensions.x),
            static_cast<int>(params.dimensions.y),
        };
        desc.setAntialiasingLevel(params.samples);
        EmplaceOrReformat(m_MaybeRenderTexture, desc);

        // update rendererd-to camera from params
        m_Camera.setPosition(params.viewPos);
        m_Camera.setNearClippingPlane(params.nearClippingPlane);
        m_Camera.setFarClippingPlane(params.farClippingPlane);
        m_Camera.setViewMatrix(params.viewMatrix);
        m_Camera.setProjectionMatrix(params.projectionMatrix);

        // handle rim highlights
        bool hasRims = false;
        glm::mat4 ndcToRimsMat{1.0f};
        glm::vec2 rimOffsets = {0.0f, 0.0f};  // in "rim space"

        if (params.drawRims)
        {
            // compute the worldspace bounds of the rim-highlighted geometry
            AABB rimAABBWorldspace = InvertedAABB();
            for (SceneDecoration const& dec : decorations)
            {
                if (dec.flags & (SceneDecorationFlags_IsSelected | SceneDecorationFlags_IsChildOfSelected | SceneDecorationFlags_IsHovered | SceneDecorationFlags_IsChildOfHovered))
                {
                    rimAABBWorldspace = Union(rimAABBWorldspace, WorldpaceAABB(dec));
                    hasRims = true;
                }
            }

            // figure out if the rims actually appear on the screen and (roughly) where
            std::optional<Rect> maybeRimRectNDC = hasRims ?
                AABBToScreenNDCRect(
                    rimAABBWorldspace,
                    m_Camera.getViewMatrix(),
                    m_Camera.getProjectionMatrix(),
                    m_Camera.getNearClippingPlane(),
                    m_Camera.getFarClippingPlane()) :
                std::nullopt;

            hasRims = maybeRimRectNDC.has_value();

            // if the scene has rims, and they can be projected onto the screen, then
            // render the rim geometry as a solid color and precompute the relevant
            // texture coordinates, quad transform, etc. for the (later) edge-detection
            // step
            if (hasRims)
            {
                Rect& rimRectNDC = *maybeRimRectNDC;

                glm::vec2 const rimThicknessNDC = 2.0f*glm::vec2{rimThickness} / glm::vec2{params.dimensions};

                // expand by the rim thickness, so that the output has space for the rims
                rimRectNDC = osc::Expand(rimRectNDC, rimThicknessNDC);

                // constrain the result of the above maths to within clip space
                rimRectNDC.p1 = glm::max(rimRectNDC.p1, glm::vec2{-1.0f, -1.0f});
                rimRectNDC.p2 = glm::min(rimRectNDC.p2, glm::vec2{1.0f, 1.0f});

                // calculate rim rect in screenspace (pixels)
                Rect const rimRectScreen = NdcRectToScreenspaceViewportRect(rimRectNDC, Rect{{}, params.dimensions});

                // calculate the dimensions of the rims in both NDC- and screen-space
                glm::vec2 const rimDimsNDC = osc::Dimensions(rimRectNDC);
                glm::vec2 const rimDimsScreen = osc::Dimensions(rimRectScreen);

                // resize the output texture (pixel) dimensions to match the (expanded) bounding rect
                RenderTextureDescriptor selectedDesc{desc};
                selectedDesc.setWidth(static_cast<int>(rimDimsScreen.x));
                selectedDesc.setHeight(static_cast<int>(rimDimsScreen.y));
                selectedDesc.setColorFormat(RenderTextureFormat::RED);
                EmplaceOrReformat(m_MaybeSelectedTexture, selectedDesc);

                // calculate a transform matrix that maps the bounding rect to the edges of clipspace
                //
                // this is so that the solid geometry render fills the output texture perfectly

                // scale output width (dims) to clipspace width (2.0f)
                glm::vec2 const scale = 2.0f / rimDimsNDC;

                // move output location to the edge of clipspace
                glm::vec2 const bottomLeftNDC = {-1.0f, -1.0f};
                glm::vec2 const position = bottomLeftNDC - (scale * glm::vec2{ glm::min(rimRectNDC.p1.x, rimRectNDC.p2.x), glm::min(rimRectNDC.p1.y, rimRectNDC.p2.y) });

                // compute transforms of the above
                Transform rimsToNDCtransform;
                rimsToNDCtransform.scale = {scale, 1.0f};
                rimsToNDCtransform.position = {position, 0.0f};

                // create matrices that render the rims to the edges of the output texture, and an inverse
                // matrix that maps this clipspace-sized quad back into its original dimensions
                glm::mat4 const rimsToNDCmat = ToMat4(rimsToNDCtransform);
                ndcToRimsMat = ToInverseMat4(rimsToNDCtransform);
                rimOffsets = rimThickness / rimDimsScreen;

                MaterialPropertyBlock selectedColor;
                selectedColor.setVec4("uDiffuseColor", {0.9f, 0.0f, 0.0f, 1.0f});
                MaterialPropertyBlock hoveredColor;
                hoveredColor.setVec4("uDiffuseColor", {0.4, 0.0f, 0.0f, 1.0f});

                // draw the solid geometry that was stretched over clipspace
                for (SceneDecoration const& dec : decorations)
                {
                    if (dec.flags & (SceneDecorationFlags_IsSelected | SceneDecorationFlags_IsChildOfSelected))
                    {
                        Graphics::DrawMesh(*dec.mesh, dec.transform, m_SolidColorMaterial, m_Camera, selectedColor);
                    }
                    else if (dec.flags & (SceneDecorationFlags_IsHovered | SceneDecorationFlags_IsChildOfHovered))
                    {
                        Graphics::DrawMesh(*dec.mesh, dec.transform, m_SolidColorMaterial, m_Camera, hoveredColor);
                    }
                }

                glm::mat4 const originalProjMat = m_Camera.getProjectionMatrix();
                m_Camera.setProjectionMatrix(rimsToNDCmat * originalProjMat);
                m_Camera.setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
                m_Camera.swapTexture(m_MaybeSelectedTexture);
                m_Camera.render();
                m_Camera.swapTexture(m_MaybeSelectedTexture);
                m_Camera.setProjectionMatrix(originalProjMat);
            }
        }

        // render scene to the screen
        {
            // draw OpenSim scene elements
            m_SceneColoredElementsMaterial.setVec3("uViewPos", m_Camera.getPosition());
            m_SceneColoredElementsMaterial.setVec3("uLightDir", params.lightDirection);
            m_SceneColoredElementsMaterial.setVec3("uLightColor", params.lightColor);
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

                if (dec.color.a > 0.99f)
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
                m_SceneTexturedElementsMaterial.setTransparent(true);  // fog

                Transform t = GetFloorTransform(params.floorLocation, params.fixupScaleFactor);

                Graphics::DrawMesh(*m_QuadMesh, t, m_SceneTexturedElementsMaterial, m_Camera);
            }

            // if rims are requested, draw them
            if (hasRims)
            {
                m_EdgeDetectorMaterial.setRenderTexture("uScreenTexture", *m_MaybeSelectedTexture);
                m_EdgeDetectorMaterial.setVec4("uRimRgba", params.rimColor);
                m_EdgeDetectorMaterial.setVec2("uRimThickness", rimOffsets);
                m_EdgeDetectorMaterial.setTransparent(true);
                m_EdgeDetectorMaterial.setDepthTested(false);

                Graphics::DrawMesh(*m_QuadMesh, m_Camera.getInverseViewProjectionMatrix() * ndcToRimsMat, m_EdgeDetectorMaterial, m_Camera);

                m_EdgeDetectorMaterial.clearRenderTexture("uScreenTexture");  // prevents copies on next frame
            }

            m_Camera.setBackgroundColor(params.backgroundColor);
            m_Camera.swapTexture(m_MaybeRenderTexture);
            m_Camera.render();
            m_Camera.swapTexture(m_MaybeRenderTexture);
        }
    }

    RenderTexture& updRenderTexture()
    {
        return m_MaybeRenderTexture.value();
    }

private:
    Material m_SceneColoredElementsMaterial
    {
        ShaderCache::get("shaders/ExperimentOpenSim.vert", "shaders/ExperimentOpenSim.frag")
    };

    Material m_SceneTexturedElementsMaterial
    {
        ShaderCache::get("shaders/ExperimentOpenSimTextured.vert", "shaders/ExperimentOpenSimTextured.frag")
    };

    Material m_SolidColorMaterial
    {
        ShaderCache::get("shaders/ExperimentOpenSimSolidColor.vert", "shaders/ExperimentOpenSimSolidColor.frag")
    };

    Material m_EdgeDetectorMaterial
    {
        ShaderCache::get("shaders/ExperimentOpenSimEdgeDetect.vert", "shaders/ExperimentOpenSimEdgeDetect.frag")
    };

    Material m_NormalsMaterial
    {
        ShaderCache::get("shaders/ExperimentGeometryShaderNormals.vert", "shaders/ExperimentGeometryShaderNormals.geom", "shaders/ExperimentGeometryShaderNormals.frag")
    };

    std::shared_ptr<Mesh const> m_QuadMesh = App::meshes().getTexturedQuadMesh();
    Texture2D m_ChequerTexture = GenChequeredFloorTexture();
    Camera m_Camera;

    // outputs
    std::optional<RenderTexture> m_MaybeSelectedTexture;
    std::optional<RenderTexture> m_MaybeRenderTexture{RenderTextureDescriptor{1, 1}};
};

osc::SceneRenderer::SceneRenderer() :
    m_Impl{new Impl{}}
{
}

osc::SceneRenderer::SceneRenderer(SceneRenderer && tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SceneRenderer& osc::SceneRenderer::operator=(SceneRenderer&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::SceneRenderer::~SceneRenderer() noexcept
{
    delete m_Impl;
}

glm::ivec2 osc::SceneRenderer::getDimensions() const
{
    return m_Impl->getDimensions();
}

int osc::SceneRenderer::getSamples() const
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
