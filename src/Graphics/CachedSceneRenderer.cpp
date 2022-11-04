#include "CachedSceneRenderer.hpp"

#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/SceneRendererParams.hpp"

#include <utility>
#include <vector>

class osc::CachedSceneRenderer::Impl final {
public:
    osc::RenderTexture& draw(nonstd::span<SceneDecoration const> decorations, SceneRendererParams const& params)
    {
        if (params != m_LastRenderingParams || !std::equal(decorations.cbegin(), decorations.cend(), m_LastDecorationList.cbegin(), m_LastDecorationList.cend()))
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

osc::CachedSceneRenderer::CachedSceneRenderer() :
    m_Impl{new Impl{}}
{
}

osc::CachedSceneRenderer::CachedSceneRenderer(CachedSceneRenderer&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::CachedSceneRenderer& osc::CachedSceneRenderer::operator=(CachedSceneRenderer&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::CachedSceneRenderer::~CachedSceneRenderer() noexcept
{
    delete m_Impl;
}

osc::RenderTexture& osc::CachedSceneRenderer::draw(nonstd::span<SceneDecoration const> decorations, SceneRendererParams const& params)
{
    return m_Impl->draw(std::move(decorations), params);
}