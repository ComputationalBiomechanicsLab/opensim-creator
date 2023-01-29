#include "CachedModelRenderer.hpp"

#include "src/Graphics/GraphicsHelpers.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneDecorationFlags.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/OpenSimBindings/Rendering/CustomDecorationOptions.hpp"
#include "src/OpenSimBindings/Rendering/CustomRenderingOptions.hpp"
#include "src/OpenSimBindings/Rendering/ModelRendererParams.hpp"
#include "src/OpenSimBindings/Rendering/OpenSimDecorationGenerator.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Utils/Perf.hpp"
#include "src/Utils/UID.hpp"

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

    // caches + versions scene state
    class CachedScene final {
    public:
        explicit CachedScene(std::shared_ptr<osc::MeshCache> meshCache) :
            m_MeshCache{std::move(meshCache)}
        {
        }

        osc::UID getVersion() const
        {
            return m_Version;
        }

        nonstd::span<osc::SceneDecoration const> getDrawlist() const
        {
            return m_Decorations;
        }

        osc::BVH const& getBVH() const
        {
            return m_BVH;
        }

        void populate(
            osc::VirtualConstModelStatePair const& msp,
            osc::CustomDecorationOptions const& decorationOptions,
            osc::CustomRenderingOptions const& renderingOptions)
        {
            OpenSim::Model const& model = msp.getModel();
            osc::UID const modelVersion = msp.getModelVersion();
            SimTK::State const& state = msp.getState();
            osc::UID const stateVersion = msp.getStateVersion();
            OpenSim::Component const* const selected = msp.getSelected();
            OpenSim::Component const* const hovered = msp.getHovered();
            float fixupFactor = msp.getFixupScaleFactor();

            if (modelVersion != m_LastModelVersion ||
                stateVersion != m_LastStateVersion ||
                selected != osc::FindComponent(model, m_LastSelection) ||
                hovered != osc::FindComponent(model, m_LastHover) ||
                fixupFactor != m_LastFixupFactor ||
                decorationOptions != m_LastDecorationOptions ||
                renderingOptions != m_LastRenderingOptions)
            {
                OSC_PERF("CachedScene/recomputeScene");

                // update cache checks
                m_LastModelVersion = modelVersion;
                m_LastStateVersion = stateVersion;
                m_LastSelection = osc::GetAbsolutePathOrEmpty(selected);
                m_LastHover = osc::GetAbsolutePathOrEmpty(hovered);
                m_LastFixupFactor = fixupFactor;
                m_LastDecorationOptions = decorationOptions;
                m_LastRenderingOptions = renderingOptions;
                m_Version.reset();

                // generate decorations from OpenSim/SimTK backend
                {
                    m_Decorations.clear();

                    OpenSim::Component const* lastComponent = nullptr;
                    osc::SceneDecorationFlags lastFlags = osc::SceneDecorationFlags_None;
                    std::string lastID;

                    osc::GenerateModelDecorations(
                        *m_MeshCache,
                        model,
                        state,
                        decorationOptions,
                        fixupFactor,
                        [this, selected, hovered, lastComponent, lastID, lastFlags](OpenSim::Component const& c, osc::SceneDecoration&& dec) mutable
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
                            m_Decorations.push_back(std::move(dec));
                        }
                    );
                }

                // create a BVH from the not-overlay parts of the scene
                osc::UpdateSceneBVH(m_Decorations, m_BVH);

                auto pushToDecorationsList = [this](osc::SceneDecoration&& dec)
                {
                    m_Decorations.push_back(std::move(dec));
                };

                // generate screen-specific overlays
                if (renderingOptions.getDrawAABBs())
                {
                    for (osc::SceneDecoration const& decoration : m_Decorations)
                    {
                        DrawAABB(*m_MeshCache, GetWorldspaceAABB(decoration), pushToDecorationsList);
                    }
                }

                if (renderingOptions.getDrawBVH())
                {
                    DrawBVH(*m_MeshCache, m_BVH, pushToDecorationsList);
                }

                if (renderingOptions.getDrawXZGrid())
                {
                    DrawXZGrid(*m_MeshCache, pushToDecorationsList);
                }

                if (renderingOptions.getDrawXYGrid())
                {
                    DrawXYGrid(*m_MeshCache, pushToDecorationsList);
                }

                if (renderingOptions.getDrawYZGrid())
                {
                    DrawYZGrid(*m_MeshCache, pushToDecorationsList);
                }

                if (renderingOptions.getDrawAxisLines())
                {
                    DrawXZFloorLines(*m_MeshCache, pushToDecorationsList);
                }
            }
        }

    private:
        std::shared_ptr<osc::MeshCache> m_MeshCache;

        osc::UID m_LastModelVersion;
        osc::UID m_LastStateVersion;
        OpenSim::ComponentPath m_LastSelection;
        OpenSim::ComponentPath m_LastHover;
        float m_LastFixupFactor = 1.0f;
        osc::CustomDecorationOptions m_LastDecorationOptions;
        osc::CustomRenderingOptions m_LastRenderingOptions;

        osc::UID m_Version;
        std::vector<osc::SceneDecoration> m_Decorations;
        osc::BVH m_BVH;
    };
}

class osc::CachedModelRenderer::Impl final {
public:
    Impl(
        Config const& config,
        std::shared_ptr<MeshCache> meshCache,
        ShaderCache& shaderCache) :

        m_Scene{meshCache},
        m_Renderer{config, *meshCache, shaderCache}
    {
    }

    void populate(
        VirtualConstModelStatePair const& modelState,
        ModelRendererParams const& params)
    {
        m_Scene.populate(modelState, params.decorationOptions, params.renderingOptions);
    }

    void draw(
        VirtualConstModelStatePair const& modelState,
        ModelRendererParams const& renderParams,
        glm::vec2 dims,
        int32_t samples)
    {
        // setup render params
        osc::SceneRendererParams params;

        if (dims.x >= 1.0f && dims.y >= 1.0f)
        {
            params.dimensions = dims;
        }
        params.samples = samples;
        params.lightDirection = osc::RecommendedLightDirection(renderParams.camera);
        params.drawFloor = renderParams.renderingOptions.getDrawFloor();
        params.viewMatrix = renderParams.camera.getViewMtx();
        params.projectionMatrix = renderParams.camera.getProjMtx(osc::AspectRatio(m_Renderer.getDimensions()));
        params.nearClippingPlane = renderParams.camera.znear;
        params.farClippingPlane = renderParams.camera.zfar;
        params.viewPos = renderParams.camera.getPos();
        params.fixupScaleFactor = modelState.getFixupScaleFactor();
        params.drawRims = renderParams.renderingOptions.getDrawSelectionRims();
        params.drawMeshNormals = renderParams.renderingOptions.getDrawMeshNormals();
        params.drawShadows = renderParams.renderingOptions.getDrawShadows();
        params.lightColor = renderParams.lightColor;
        params.backgroundColor = renderParams.backgroundColor;
        params.floorLocation = renderParams.floorLocation;

        // todo: separate population, parameter generation, drawlist yielding, etc. so that
        // the state machine can play with stuff, etc?

        if (m_Scene.getVersion() != m_RendererPrevDrawlistVersion ||
            params != m_RendererPrevParams)
        {
            m_RendererPrevDrawlistVersion = m_Scene.getVersion();
            m_RendererPrevParams = params;
            m_Renderer.draw(m_Scene.getDrawlist(), params);
        }
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
        if (m_Scene.getBVH().nodes.empty())
        {
            return std::nullopt;
        }
        else
        {
            return m_Scene.getBVH().nodes[0].getBounds();
        }
    }

    std::vector<SceneCollision> getAllSceneCollisions(Line const& worldspaceRay) const
    {
        // get decorations list (used for later testing/filtering)
        nonstd::span<osc::SceneDecoration const> decorations = m_Scene.getDrawlist();

        // find all collisions along the camera ray
        return osc::GetAllSceneCollisions(m_Scene.getBVH(), decorations, worldspaceRay);
    }

private:
    CachedScene m_Scene;

    // rendering input state
    osc::SceneRendererParams m_RendererPrevParams;
    osc::UID m_RendererPrevDrawlistVersion;
    osc::SceneRenderer m_Renderer;
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

void osc::CachedModelRenderer::populate(
    VirtualConstModelStatePair const& modelState,
    ModelRendererParams const& params)
{
    m_Impl->populate(modelState, params);
}

void osc::CachedModelRenderer::draw(
    VirtualConstModelStatePair const& modelState,
    ModelRendererParams const& renderParams,
    glm::vec2 dims,
    int32_t samples)
{
    m_Impl->draw(modelState, renderParams, dims, samples);
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

std::vector<osc::SceneCollision> osc::CachedModelRenderer::getAllSceneCollisions(Line const& worldspaceRay) const
{
    return m_Impl->getAllSceneCollisions(worldspaceRay);
}