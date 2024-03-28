#include "CachedSceneRenderer.h"

#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Graphics/Scene/SceneRenderer.h>
#include <oscar/Graphics/Scene/SceneRendererParams.h>
#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <memory>
#include <span>
#include <vector>

using namespace osc;

class osc::CachedSceneRenderer::Impl final {
public:
    explicit Impl(SceneCache& scene_cache) :
        scene_renderer_{scene_cache}
    {}

    RenderTexture& render(
        std::span<const SceneDecoration> decorations,
        const SceneRendererParams& params)
    {
        if (params != last_rendering_params_ or not equal(decorations, last_decoration_list_)) {

            // inputs have changed: cache the new ones and re-render
            last_rendering_params_ = params;
            last_decoration_list_.assign(decorations.begin(), decorations.end());
            scene_renderer_.render(last_decoration_list_, last_rendering_params_);
        }

        return scene_renderer_.upd_render_texture();
    }

private:
    SceneRendererParams last_rendering_params_;
    std::vector<SceneDecoration> last_decoration_list_;
    SceneRenderer scene_renderer_;
};


// public API (PIMPL)

osc::CachedSceneRenderer::CachedSceneRenderer(SceneCache& scene_cache) :
    m_Impl{std::make_unique<Impl>(scene_cache)}
{}

osc::CachedSceneRenderer::CachedSceneRenderer(CachedSceneRenderer&&) noexcept = default;
osc::CachedSceneRenderer& osc::CachedSceneRenderer::operator=(CachedSceneRenderer&&) noexcept = default;
osc::CachedSceneRenderer::~CachedSceneRenderer() noexcept = default;

RenderTexture& osc::CachedSceneRenderer::render(
    std::span<const SceneDecoration> decorations,
    const SceneRendererParams& params)
{
    return m_Impl->render(decorations, params);
}
