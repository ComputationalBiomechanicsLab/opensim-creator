#pragma once

#include <memory>
#include <span>

namespace osc { class RenderTexture; }
namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }
namespace osc { struct SceneRendererParams; }

namespace osc
{
    // a cached version of `SceneRenderer` that will re-render if either
    // the `SceneDecoration`s or the `SceneRendererParams` change
    class CachedSceneRenderer final {
    public:
        explicit CachedSceneRenderer(SceneCache&);
        CachedSceneRenderer(const CachedSceneRenderer&) = delete;
        CachedSceneRenderer(CachedSceneRenderer&&) noexcept;
        CachedSceneRenderer& operator=(const CachedSceneRenderer&) = delete;
        CachedSceneRenderer& operator=(CachedSceneRenderer&&) noexcept;
        ~CachedSceneRenderer() noexcept;

        RenderTexture& render(std::span<const SceneDecoration>, const SceneRendererParams&);

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
