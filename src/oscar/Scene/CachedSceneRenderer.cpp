#include "CachedSceneRenderer.hpp"

#include <oscar/Scene/SceneDecoration.hpp>
#include <oscar/Scene/SceneRenderer.hpp>
#include <oscar/Scene/SceneRendererParams.hpp>

#include <algorithm>
#include <memory>
#include <span>
#include <vector>

using namespace osc;

class osc::CachedSceneRenderer::Impl final {
public:
    Impl(
        AppConfig const& config,
        SceneCache& meshCache,
        ShaderCache& shaderCache) :

        m_SceneRenderer{config, meshCache, shaderCache}
    {
    }

    RenderTexture& render(
        std::span<SceneDecoration const> decorations,
        SceneRendererParams const& params)
    {
        if (params != m_LastRenderingParams ||
            !std::equal(decorations.begin(), decorations.end(), m_LastDecorationList.cbegin(), m_LastDecorationList.cend()))
        {
            // inputs have changed: cache the new ones and re-render
            m_LastRenderingParams = params;
            m_LastDecorationList.assign(decorations.begin(), decorations.end());
            m_SceneRenderer.render(m_LastDecorationList, m_LastRenderingParams);
        }

        return m_SceneRenderer.updRenderTexture();
    }

private:
    SceneRendererParams m_LastRenderingParams;
    std::vector<SceneDecoration> m_LastDecorationList;
    SceneRenderer m_SceneRenderer;
};


// public API (PIMPL)

osc::CachedSceneRenderer::CachedSceneRenderer(
    AppConfig const& config,
    SceneCache& meshCache,
    ShaderCache& shaderCache) :
    m_Impl{std::make_unique<Impl>(config, meshCache, shaderCache)}
{
}

osc::CachedSceneRenderer::CachedSceneRenderer(CachedSceneRenderer&&) noexcept = default;
osc::CachedSceneRenderer& osc::CachedSceneRenderer::operator=(CachedSceneRenderer&&) noexcept = default;
osc::CachedSceneRenderer::~CachedSceneRenderer() noexcept = default;

RenderTexture& osc::CachedSceneRenderer::render(
    std::span<SceneDecoration const> decorations,
    SceneRendererParams const& params)
{
    return m_Impl->render(decorations, params);
}
