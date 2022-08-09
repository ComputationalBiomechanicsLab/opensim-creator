#pragma once

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

namespace osc
{
    class SceneDecoration;
    class SceneRendererParams;
}

namespace osc::experimental
{
    class RenderTexture;
}

namespace osc
{
    class SceneRenderer final {
    public:
        SceneRenderer();
        SceneRenderer(SceneRenderer const&) = delete;
        SceneRenderer(SceneRenderer&&) noexcept;
        SceneRenderer& operator=(SceneRenderer const&) = delete;
        SceneRenderer& operator=(SceneRenderer&&) noexcept;
        ~SceneRenderer() noexcept;

        glm::ivec2 getDimensions() const;
        int getSamples() const;
        void draw(nonstd::span<SceneDecoration const>, SceneRendererParams const&);
        experimental::RenderTexture& updRenderTexture();

    private:
        class Impl;
        Impl* m_Impl;
    };
}