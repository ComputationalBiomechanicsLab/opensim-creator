#pragma once

#include <nonstd/span.hpp>

#include <memory>

namespace osc { class MeshCache; }
namespace osc { class RenderTexture; }
namespace osc { class SceneDecoration; }
namespace osc { class SceneRendererParams; }
namespace osc { class ShaderCache; }

namespace osc
{
    // a scene renderer that only renders if the render parameters + decorations change
    class CachedSceneRenderer final {
    public:
        CachedSceneRenderer(MeshCache&, ShaderCache&);
        CachedSceneRenderer(CachedSceneRenderer const&) = delete;
        CachedSceneRenderer(CachedSceneRenderer&&) noexcept;
        CachedSceneRenderer& operator=(CachedSceneRenderer const&) = delete;
        CachedSceneRenderer& operator=(CachedSceneRenderer&&) noexcept;
        ~CachedSceneRenderer() noexcept;

        RenderTexture& draw(nonstd::span<SceneDecoration const>, SceneRendererParams const&);

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}