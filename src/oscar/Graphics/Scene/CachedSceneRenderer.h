#pragma once

#include <memory>
#include <span>

namespace osc { class RenderTexture; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }
namespace osc { struct SceneRendererParams; }

namespace osc
{
    // a scene renderer that only renders if the render parameters + decorations change
    class CachedSceneRenderer final {
    public:
        CachedSceneRenderer(SceneCache&);
        CachedSceneRenderer(CachedSceneRenderer const&) = delete;
        CachedSceneRenderer(CachedSceneRenderer&&) noexcept;
        CachedSceneRenderer& operator=(CachedSceneRenderer const&) = delete;
        CachedSceneRenderer& operator=(CachedSceneRenderer&&) noexcept;
        ~CachedSceneRenderer() noexcept;

        RenderTexture& render(std::span<SceneDecoration const>, SceneRendererParams const&);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
