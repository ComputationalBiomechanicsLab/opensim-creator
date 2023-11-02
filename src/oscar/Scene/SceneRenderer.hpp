#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Maths/Vec2.hpp>

#include <nonstd/span.hpp>

#include <memory>

namespace osc { class AppConfig; }
namespace osc { class MeshCache; }
namespace osc { class ShaderCache; }
namespace osc { struct SceneDecoration; }
namespace osc { struct SceneRendererParams; }
namespace osc { class RenderTexture; }

namespace osc
{
    class SceneRenderer final {
    public:
        SceneRenderer(AppConfig const&, MeshCache&, ShaderCache&);
        SceneRenderer(SceneRenderer const&);
        SceneRenderer(SceneRenderer&&) noexcept;
        SceneRenderer& operator=(SceneRenderer const&) = delete;
        SceneRenderer& operator=(SceneRenderer&&) noexcept;
        ~SceneRenderer() noexcept;

        Vec2i getDimensions() const;
        AntiAliasingLevel getAntiAliasingLevel() const;
        void render(nonstd::span<SceneDecoration const>, SceneRendererParams const&);
        RenderTexture& updRenderTexture();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
