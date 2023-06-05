#include "CachedModelRenderer.hpp"

#include "OpenSimCreator/Graphics/CustomDecorationOptions.hpp"
#include "OpenSimCreator/Graphics/CustomRenderingOptions.hpp"
#include "OpenSimCreator/Graphics/ModelRendererParams.hpp"
#include "OpenSimCreator/Graphics/ModelSceneDecorations.hpp"
#include "OpenSimCreator/Graphics/ModelSceneDecorationsParams.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp"
#include "OpenSimCreator/Graphics/OverlayDecorationGenerator.hpp"
#include "OpenSimCreator/OpenSimHelpers.hpp"
#include "OpenSimCreator/VirtualConstModelStatePair.hpp"

#include <oscar/Graphics/GraphicsHelpers.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/SceneDecoration.hpp>
#include <oscar/Graphics/SceneDecorationFlags.hpp>
#include <oscar/Graphics/SceneRenderer.hpp>
#include <oscar/Maths/BVH.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/PolarPerspectiveCamera.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Utils/Perf.hpp>
#include <oscar/Utils/UID.hpp>

#include <nonstd/span.hpp>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace OpenSim { class Component; }

namespace
{
    // helper: compute the decoration flags for a given component
    osc::SceneDecorationFlags ComputeSceneDecorationFlags(
        OpenSim::Component const& component,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered)
    {
        osc::SceneDecorationFlags rv = osc::SceneDecorationFlags_CastsShadows;

        if (&component == selected)
        {
            rv |= osc::SceneDecorationFlags_IsSelected;
        }

        if (&component == hovered)
        {
            rv |= osc::SceneDecorationFlags_IsHovered;
        }

        OpenSim::Component const* ptr = osc::GetOwner(component);
        while (ptr)
        {
            if (ptr == selected)
            {
                rv |= osc::SceneDecorationFlags_IsChildOfSelected;
            }
            if (ptr == hovered)
            {
                rv |= osc::SceneDecorationFlags_IsChildOfHovered;
            }
            ptr = osc::GetOwner(*ptr);
        }

        return rv;
    }

    // auto-focus the given camera on the given decorations
    void AutoFocus(
        osc::ModelSceneDecorations const& decorations,
        float aspectRatio,
        osc::PolarPerspectiveCamera& camera)
    {
        if (std::optional<osc::AABB> const root = decorations.getRootAABB())
        {
            osc::AutoFocus(camera, *root, aspectRatio);
        }
    }

    // generate model decorations via the SimTK/OpenSim backend
    void GenerateModelDecorations(
        osc::VirtualConstModelStatePair const& model,
        osc::ModelSceneDecorationsParams const& params,
        osc::MeshCache& meshCache,
        osc::ModelSceneDecorations& out)
    {
        OpenSim::Component const* selected = model.getSelected();
        OpenSim::Component const* hovered = model.getHovered();
        OpenSim::Component const* lastComponent = nullptr;
        osc::SceneDecorationFlags lastFlags = osc::SceneDecorationFlags_None;
        std::string lastID;

        osc::GenerateModelDecorations(
            meshCache,
            model.getModel(),
            model.getState(),
            params.decorationOptions,
            params.fixupScaleFactor,
            [&out, selected, hovered, lastComponent, lastID, lastFlags](OpenSim::Component const& c, osc::SceneDecoration&& dec) mutable
            {
                if (&c == lastComponent)
                {
                    dec.id = lastID;
                    dec.flags = lastFlags;
                }
                else
                {
                    osc::GetAbsolutePathString(c, lastID);
                    dec.id = lastID;
                    dec.flags = ComputeSceneDecorationFlags(c, selected, hovered);
                    lastFlags = dec.flags;
                    lastComponent = &c;
                }
                out.push_back(std::move(dec));
            }
        );
    }

    // generate all model scene decorations for the given model+state pair and parameters
    void GenerateModelSceneDecorations(
        osc::VirtualConstModelStatePair const& model,
        osc::ModelSceneDecorationsParams const& params,
        osc::MeshCache& meshCache,
        osc::ModelSceneDecorations& out)
    {
        out.clear();
        GenerateModelDecorations(model, params, meshCache, out);
        out.computeBVH();  // only hittest model decorations
        auto onAppend = [&out](osc::SceneDecoration&& dec) { out.push_back(std::move(dec)); };
        osc::GenerateOverlayDecorations(meshCache, params.renderingOptions, out.getBVH(), onAppend);
    }

    // create low-level scene renderer parameters from the given high-level model
    // rendering parameters
    osc::SceneRendererParams CreateSceneRenderParams(
        glm::vec2 dims,
        int32_t samples,
        osc::ModelRendererParams const& renderParams,
        float fixupScaleFactor)
    {
        osc::SceneRendererParams params;
        if (dims.x >= 1.0f && dims.y >= 1.0f)
        {
            params.dimensions = dims;
        }
        params.samples = samples;
        params.lightDirection = osc::RecommendedLightDirection(renderParams.camera);
        params.drawFloor = renderParams.renderingOptions.getDrawFloor();
        params.viewMatrix = renderParams.camera.getViewMtx();
        params.projectionMatrix = renderParams.camera.getProjMtx(osc::AspectRatio(dims));
        params.nearClippingPlane = renderParams.camera.znear;
        params.farClippingPlane = renderParams.camera.zfar;
        params.viewPos = renderParams.camera.getPos();
        params.fixupScaleFactor = fixupScaleFactor;
        params.drawRims = renderParams.renderingOptions.getDrawSelectionRims();
        params.drawMeshNormals = renderParams.renderingOptions.getDrawMeshNormals();
        params.drawShadows = renderParams.renderingOptions.getDrawShadows();
        params.lightColor = renderParams.lightColor;
        params.backgroundColor = renderParams.backgroundColor;
        params.floorLocation = renderParams.floorLocation;
        return params;
    }
}

class osc::CachedModelRenderer::Impl final {
public:
    Impl(
        Config const& config,
        std::shared_ptr<MeshCache> meshCache,
        ShaderCache& shaderCache) :

        m_MeshCache{meshCache},
        m_Renderer{config, *meshCache, shaderCache}
    {
    }

    void autoFocusCamera(
        VirtualConstModelStatePair const& modelState,
        ModelRendererParams& params,
        float aspectRatio)
    {
        generateDecorationsCached(modelState, params);
        ::AutoFocus(m_Scene, aspectRatio, params.camera);
    }

    RenderTexture& draw(
        VirtualConstModelStatePair const& modelState,
        ModelRendererParams const& renderParams,
        glm::vec2 dims,
        int32_t samples)
    {
        OSC_PERF("CachedModelRenderer/draw");

        // setup render/rasterization parameters
        osc::SceneRendererParams const rendererParameters = CreateSceneRenderParams(
            dims,
            samples,
            renderParams,
            modelState.getFixupScaleFactor()
        );

        // if the decorations or rendering params have changed, re-render
        if (generateDecorationsCached(modelState, renderParams) ||
            rendererParameters != m_PrevRendererParams)
        {
            OSC_PERF("CachedModelRenderer/draw/render");
            m_Renderer.draw(m_Scene.getDrawlist(), rendererParameters);
            m_PrevRendererParams = rendererParameters;
        }

        return m_Renderer.updRenderTexture();
    }

    RenderTexture& updRenderTexture()
    {
        return m_Renderer.updRenderTexture();
    }

    nonstd::span<SceneDecoration const> getDrawlist() const
    {
        return m_Scene.getDrawlist();
    }

    std::optional<AABB> getRootAABB() const
    {
        return m_Scene.getRootAABB();
    }

    std::optional<SceneCollision> getClosestCollision(
        ModelRendererParams const& params,
        glm::vec2 mouseScreenPos,
        Rect const& viewportScreenRect) const
    {
        return m_Scene.getClosestCollision(
            params.camera,
            mouseScreenPos,
            viewportScreenRect
        );
    }

private:
    bool generateDecorationsCached(
        VirtualConstModelStatePair const& modelState,
        ModelRendererParams const& renderParams)
    {
        OSC_PERF("CachedModelRenderer/generateDecorationsCached");

        ModelSceneDecorationsParams decorationParams
        {
            modelState,
            renderParams.decorationOptions,
            renderParams.renderingOptions
        };

        if (decorationParams == m_PrevSceneParams)
        {
            return false;  // no decoration generation necessary
        }
        else
        {
            GenerateModelSceneDecorations(modelState, decorationParams, *m_MeshCache, m_Scene);
            m_PrevSceneParams = std::move(decorationParams);
            return true;  // new decorations were generated
        }
    }

    std::shared_ptr<MeshCache> m_MeshCache;
    ModelSceneDecorationsParams m_PrevSceneParams;
    ModelSceneDecorations m_Scene;
    SceneRendererParams m_PrevRendererParams;
    SceneRenderer m_Renderer;
};


// public API (PIMPL)

osc::CachedModelRenderer::CachedModelRenderer(
    Config const& config,
    std::shared_ptr<MeshCache> meshCache,
    ShaderCache& shaderCache) :

    m_Impl{std::make_unique<Impl>(config, std::move(meshCache), shaderCache)}
{
}
osc::CachedModelRenderer::CachedModelRenderer(CachedModelRenderer&&) noexcept = default;
osc::CachedModelRenderer& osc::CachedModelRenderer::operator=(CachedModelRenderer&&) noexcept = default;
osc::CachedModelRenderer::~CachedModelRenderer() noexcept = default;

osc::RenderTexture& osc::CachedModelRenderer::draw(
    VirtualConstModelStatePair const& modelState,
    ModelRendererParams const& renderParams,
    glm::vec2 dims,
    int32_t samples)
{
    return m_Impl->draw(
        modelState,
        renderParams,
        dims,
        samples
    );
}

void osc::CachedModelRenderer::autoFocusCamera(
    VirtualConstModelStatePair const& modelState,
    ModelRendererParams& renderParams,
    float aspectRatio)
{
    m_Impl->autoFocusCamera(modelState, renderParams, aspectRatio);
}

osc::RenderTexture& osc::CachedModelRenderer::updRenderTexture()
{
    return m_Impl->updRenderTexture();
}

nonstd::span<osc::SceneDecoration const> osc::CachedModelRenderer::getDrawlist() const
{
    return m_Impl->getDrawlist();
}

std::optional<osc::AABB> osc::CachedModelRenderer::getRootAABB() const
{
    return m_Impl->getRootAABB();
}

std::optional<osc::SceneCollision> osc::CachedModelRenderer::getClosestCollision(
    ModelRendererParams const& params,
    glm::vec2 mouseScreenPos,
    Rect const& viewportScreenRect) const
{
    return m_Impl->getClosestCollision(params, mouseScreenPos, viewportScreenRect);
}