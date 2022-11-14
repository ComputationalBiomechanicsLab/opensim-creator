#pragma once

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <memory>

namespace osc { class SceneDecoration; }
namespace osc { class SceneRendererParams; }
namespace osc { class RenderTexture; }

namespace osc
{
    class SceneRenderer final {
    public:
        SceneRenderer();
        SceneRenderer(SceneRenderer const&);
        SceneRenderer(SceneRenderer&&) noexcept;
        SceneRenderer& operator=(SceneRenderer const&) = delete;
        SceneRenderer& operator=(SceneRenderer&&) noexcept;
        ~SceneRenderer() noexcept;

        glm::ivec2 getDimensions() const;
        int32_t getSamples() const;
        void draw(nonstd::span<SceneDecoration const>, SceneRendererParams const&);
        RenderTexture& updRenderTexture();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}