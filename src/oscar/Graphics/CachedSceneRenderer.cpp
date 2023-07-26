#include "CachedSceneRenderer.hpp"

#include "oscar/Graphics/SceneDecoration.hpp"
#include "oscar/Graphics/SceneRenderer.hpp"
#include "oscar/Graphics/SceneRendererParams.hpp"
#include "oscar/Platform/Config.hpp"

#include <utility>
#include <vector>

class osc::CachedSceneRenderer::Impl final {
public:
    Impl(
        Config const& config,
        MeshCache& meshCache,
        ShaderCache& shaderCache) :

        m_SceneRenderer{config, meshCache, shaderCache}
    {
    }

    osc::RenderTexture& draw(
        nonstd::span<SceneDecoration const> decorations,
        SceneRendererParams const& params)
    {
        if (params != m_LastRenderingParams ||
            !std::equal(decorations.begin(), decorations.end(), m_LastDecorationList.cbegin(), m_LastDecorationList.cend()))
        {
            // inputs have changed: cache the new ones and re-render
            m_LastRenderingParams = params;
            m_LastDecorationList.assign(decorations.begin(), decorations.end());
            m_SceneRenderer.draw(m_LastDecorationList, m_LastRenderingParams);
        }

        return m_SceneRenderer.updRenderTexture();
    }

private:
    osc::SceneRendererParams m_LastRenderingParams;
    std::vector<osc::SceneDecoration> m_LastDecorationList;
    osc::SceneRenderer m_SceneRenderer;
};


// public API (PIMPL)

osc::CachedSceneRenderer::CachedSceneRenderer(Config const& config, MeshCache& meshCache, ShaderCache& shaderCache) :
    m_Impl{std::make_unique<Impl>(config, meshCache, shaderCache)}
{
}

osc::CachedSceneRenderer::CachedSceneRenderer(CachedSceneRenderer&&) noexcept = default;
osc::CachedSceneRenderer& osc::CachedSceneRenderer::operator=(CachedSceneRenderer&&) noexcept = default;
osc::CachedSceneRenderer::~CachedSceneRenderer() noexcept = default;

osc::RenderTexture& osc::CachedSceneRenderer::draw(
    nonstd::span<SceneDecoration const> decorations,
    SceneRendererParams const& params)
{
    return m_Impl->draw(decorations, params);
}
