#include "CachedModelRenderer.hpp"

#include "OpenSimCreator/Graphics/CustomDecorationOptions.hpp"
#include "OpenSimCreator/Graphics/CustomRenderingOptions.hpp"
#include "OpenSimCreator/Graphics/ModelRendererParams.hpp"
#include "OpenSimCreator/Graphics/OpenSimDecorationGenerator.hpp"
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
    osc::SceneDecorationFlags ComputeFlags(
        OpenSim::Component const& c,
        OpenSim::Component const* selected,
        OpenSim::Component const* hovered)
    {
        osc::SceneDecorationFlags rv = osc::SceneDecorationFlags_CastsShadows;

        if (&c == selected)
        {
            rv |= osc::SceneDecorationFlags_IsSelected;
        }

        if (&c == hovered)
        {
            rv |= osc::SceneDecorationFlags_IsHovered;
        }

        OpenSim::Component const* ptr = osc::GetOwner(c);
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

    struct ModelDecorationsParams final {
        ModelDecorationsParams() = default;

        ModelDecorationsParams(
            osc::VirtualConstModelStatePair const& msp_,
            osc::CustomDecorationOptions const& decorationOptions_,
            osc::CustomRenderingOptions const& renderingOptions_) :

            modelVersion{msp_.getModelVersion()},
            stateVersion{msp_.getStateVersion()},
            selection{osc::GetAbsolutePathOrEmpty(msp_.getSelected())},
            hover{osc::GetAbsolutePathOrEmpty(msp_.getHovered())},
            fixupScaleFactor{msp_.getFixupScaleFactor()},
            decorationOptions{decorationOptions_},
            renderingOptions{renderingOptions_}
        {
        }

    private:
        friend bool operator==(ModelDecorationsParams const&, ModelDecorationsParams const&) noexcept;
        osc::UID modelVersion;
        osc::UID stateVersion;
    public:
        OpenSim::ComponentPath selection;
        OpenSim::ComponentPath hover;
        float fixupScaleFactor = 1.0f;
        osc::CustomDecorationOptions decorationOptions;
        osc::CustomRenderingOptions renderingOptions;
    };

    bool operator==(ModelDecorationsParams const& a, ModelDecorationsParams const& b) noexcept
    {
        return
            a.modelVersion == b.modelVersion &&
            a.stateVersion == b.stateVersion &&
            a.selection == b.selection &&
            a.hover == b.hover &&
            a.fixupScaleFactor == b.fixupScaleFactor &&
            a.decorationOptions == b.decorationOptions &&
            a.renderingOptions == b.renderingOptions;
    }

    bool operator!=(ModelDecorationsParams const& a, ModelDecorationsParams const& b)
    {
        return !(a == b);
    }

    struct ModelDecorations final {
        std::vector<osc::SceneDecoration> drawlist;
        osc::BVH bvh;
    };

    // generate model decorations via the SimTK/OpenSim backend
    void GenerateModelDecorations(
        std::vector<osc::SceneDecoration>& drawlist,
        osc::MeshCache& meshCache,
        osc::VirtualConstModelStatePair const& model,
        osc::CustomDecorationOptions const& decorationOptions,
        float fixupScaleFactor)
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
            decorationOptions,
            fixupScaleFactor,
            [&drawlist, selected, hovered, lastComponent, lastID, lastFlags](OpenSim::Component const& c, osc::SceneDecoration&& dec) mutable
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
                    dec.flags = ComputeFlags(c, selected, hovered);
                    lastFlags = dec.flags;
                    lastComponent = &c;
                }
                drawlist.push_back(std::move(dec));
            }
        );
    }

    void GenerateOverlayDecorations(
        ModelDecorations& decorations,
        osc::MeshCache& meshCache,
        ModelDecorationsParams const& params)
    {
        auto pushToDecorationsList = [&decorations](osc::SceneDecoration&& dec)
        {
            decorations.drawlist.push_back(std::move(dec));
        };

        // generate screen-specific overlays
        if (params.renderingOptions.getDrawAABBs())
        {
            // likely guess: each decoration will have one AABB
            decorations.drawlist.reserve(2*decorations.drawlist.size());

            // CARE: iterators may be invalidated here, because DrawAABB is also
            //       adding to the list that's being iterated over
            //
            //       so, to prevent a segfault etc., you *must* cache the index
            for (size_t i = 0, len = decorations.drawlist.size(); i < len; ++i)
            {
                osc::DrawAABB(
                    meshCache,
                    osc::GetWorldspaceAABB(decorations.drawlist[i]),
                    pushToDecorationsList
                );
            }
        }

        if (params.renderingOptions.getDrawBVH())
        {
            osc::DrawBVH(meshCache, decorations.bvh, pushToDecorationsList);
        }

        if (params.renderingOptions.getDrawXZGrid())
        {
            osc::DrawXZGrid(meshCache, pushToDecorationsList);
        }

        if (params.renderingOptions.getDrawXYGrid())
        {
            osc::DrawXYGrid(meshCache, pushToDecorationsList);
        }

        if (params.renderingOptions.getDrawYZGrid())
        {
            osc::DrawYZGrid(meshCache, pushToDecorationsList);
        }

        if (params.renderingOptions.getDrawAxisLines())
        {
            osc::DrawXZFloorLines(meshCache, pushToDecorationsList);
        }
    }

    void GenerateSceneDecorations(
        ModelDecorations& decorations,
        osc::MeshCache& meshCache,
        osc::VirtualConstModelStatePair const& model,
        ModelDecorationsParams const& params)
    {
        decorations.drawlist.clear();

        // generate model decorations from OpenSim/SimTK backend
        GenerateModelDecorations(
            decorations.drawlist,
            meshCache,
            model,
            params.decorationOptions,
            params.fixupScaleFactor
        );

        // create a BVH from the not-overlay parts of the scene
        osc::UpdateSceneBVH(
            decorations.drawlist,
            decorations.bvh
        );

        // then add any overlay decorations that aren't subject to the
        // hittest (e.g. grids, AABBs)
        GenerateOverlayDecorations(decorations, meshCache, params);
    }

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

    bool IsSceneDecorationIDed(osc::SceneDecoration const& dec)
    {
        return !dec.id.empty();
    }

    std::optional<osc::SceneCollision> GetClosestCollision(
        osc::Line const& worldspaceRay,
        std::function<bool(osc::SceneDecoration const&)> const& filter,
        nonstd::span<osc::SceneDecoration const> drawlist,
        osc::BVH const& sceneBVH)
    {
        // find all collisions along the camera ray
        std::vector<osc::SceneCollision> collisions = GetAllSceneCollisions(sceneBVH, drawlist, worldspaceRay);

        // filter through the collisions list
        osc::SceneCollision const* closestCollision = nullptr;
        for (osc::SceneCollision const& c : collisions)
        {
            if (closestCollision && c.distanceFromRayOrigin > closestCollision->distanceFromRayOrigin)
            {
                continue;  // it's further away than the current closest collision
            }

            osc::SceneDecoration const& decoration = drawlist[c.decorationIndex];

            if (!filter(decoration))
            {
                continue;  // filtered out by external filter
            }

            closestCollision = &c;
        }

        if (closestCollision)
        {
            return *closestCollision;
        }
        else
        {
            return std::nullopt;
        }
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
        if (std::optional<AABB> const root = getRootAABB())
        {
            AutoFocus(params.camera, *root, aspectRatio);
        }
    }

    RenderTexture& draw(
        VirtualConstModelStatePair const& modelState,
        ModelRendererParams const& renderParams,
        glm::vec2 dims,
        int32_t samples)
    {
        OSC_PERF("CachedModelRenderer/draw");

        // generate scene decorations
        DecorationUpdateResponse const decorationUpdateResponse = generateDecorationsCached(modelState, renderParams);

        // setup render/rasterization parameters
        osc::SceneRendererParams const rendererParameters = CreateSceneRenderParams(
            dims,
            samples,
            renderParams,
            modelState.getFixupScaleFactor()
        );

        // if the decoration/params have changed, re-render
        if (decorationUpdateResponse == DecorationUpdateResponse::Generated ||
            rendererParameters != m_PrevRendererParams)
        {
            OSC_PERF("CachedModelRenderer/draw/render");
            m_Renderer.draw(m_Scene.drawlist, rendererParameters);
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
        return m_Scene.drawlist;
    }

    std::optional<AABB> getRootAABB() const
    {
        if (m_Scene.bvh.nodes.empty())
        {
            return std::nullopt;
        }
        else
        {
            return m_Scene.bvh.nodes.front().getBounds();
        }
    }

    std::optional<SceneCollision> getClosestCollision(
        ModelRendererParams const& params,
        glm::vec2 mouseScreenPos,
        Rect const& viewportScreenRect) const
    {
        OSC_PERF("CachedModelRenderer/getClosestCollision");

        // un-project 2D mouse cursor into 3D scene as a ray
        glm::vec2 const mouseRenderPos = mouseScreenPos - viewportScreenRect.p1;
        Line const worldspaceCameraRay = params.camera.unprojectTopLeftPosToWorldRay(
            mouseRenderPos,
            Dimensions(viewportScreenRect)
        );

        return GetClosestCollision(
            worldspaceCameraRay,
            IsSceneDecorationIDed,  // only hittest IDed elements
            m_Scene.drawlist,
            m_Scene.bvh
        );
    }

private:
    enum DecorationUpdateResponse { Cached, Generated };

    DecorationUpdateResponse generateDecorationsCached(
        VirtualConstModelStatePair const& msp,
        ModelRendererParams const& renderParams)
    {
        OSC_PERF("CachedModelRenderer/generateDecorationsCached");

        ModelDecorationsParams params
        {
            msp,
            renderParams.decorationOptions,
            renderParams.renderingOptions
        };
        if (params == m_PrevSceneParams)
        {
            return DecorationUpdateResponse::Cached;
        }
        else
        {
            GenerateSceneDecorations(m_Scene, *m_MeshCache, msp, params);
            m_PrevSceneParams = std::move(params);
            return DecorationUpdateResponse::Generated;
        }
    }

    std::shared_ptr<MeshCache> m_MeshCache;

    ModelDecorationsParams m_PrevSceneParams;
    ModelDecorations m_Scene;

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