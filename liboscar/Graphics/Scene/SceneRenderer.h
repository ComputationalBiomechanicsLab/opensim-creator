#pragma once

#include <memory>
#include <span>

namespace osc { struct SceneDecoration; }
namespace osc { class SceneCache; }
namespace osc { struct SceneRendererParams; }
namespace osc { class RenderTexture; }

namespace osc
{
    // renders the given `SceneDecoration`s with the given `SceneRendererParams`
    // to a rasterized `RenderTexture`
    class SceneRenderer final {
    public:
        explicit SceneRenderer(SceneCache&);
        SceneRenderer(const SceneRenderer&);
        SceneRenderer(SceneRenderer&&) noexcept;
        SceneRenderer& operator=(const SceneRenderer&) = delete;
        SceneRenderer& operator=(SceneRenderer&&) noexcept;
        ~SceneRenderer() noexcept;

        void render(std::span<const SceneDecoration>, const SceneRendererParams&);
        RenderTexture& upd_render_texture();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
