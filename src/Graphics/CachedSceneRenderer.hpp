#pragma once

#include <nonstd/span.hpp>

namespace osc { class RenderTexture; }
namespace osc { class SceneDecoration; }
namespace osc { class SceneRendererParams; }

namespace osc
{
    // a scene renderer that only renders if the render parameters + decorations change
    class CachedSceneRenderer final {
    public:
        CachedSceneRenderer();
        CachedSceneRenderer(CachedSceneRenderer const&) = delete;
        CachedSceneRenderer(CachedSceneRenderer&&) noexcept;
        CachedSceneRenderer& operator=(CachedSceneRenderer const&) = delete;
        CachedSceneRenderer& operator=(CachedSceneRenderer&&) noexcept;
        ~CachedSceneRenderer() noexcept;

        RenderTexture& draw(nonstd::span<SceneDecoration const>, SceneRendererParams const&);

    private:
        class Impl;
        Impl* m_Impl;
    };
}