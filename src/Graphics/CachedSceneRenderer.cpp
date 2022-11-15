#include "CachedSceneRenderer.hpp"

#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Platform/Config.hpp"

#include <utility>
#include <vector>

class osc::CachedSceneRenderer::Impl final {
public:
    Impl(Config const& config, MeshCache& meshCache, ShaderCache& shaderCache) :
        m_LastRenderingParams{},
        m_LastDecorationList{},
        m_SceneRenderer{config, meshCache, shaderCache}
    {
    }

    osc::RenderTexture& draw(nonstd::span<SceneDecoration const> decorations, SceneRendererParams const& params)
    {
        if (params != m_LastRenderingParams ||
            !std::equal(decorations.cbegin(), decorations.cend(), m_LastDecorationList.cbegin(), m_LastDecorationList.cend()))
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

osc::RenderTexture& osc::CachedSceneRenderer::draw(nonstd::span<SceneDecoration const> decorations, SceneRendererParams const& params)
{
    return m_Impl->draw(std::move(decorations), params);
}