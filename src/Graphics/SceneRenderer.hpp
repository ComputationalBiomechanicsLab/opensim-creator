#pragma once

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

namespace gl
{
    class FrameBuffer;
    class Texture2D;
}

namespace osc
{
    class SceneDecorationNew;
    class SceneRendererParams;
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
        void draw(nonstd::span<SceneDecorationNew const>, SceneRendererParams const&);
        gl::Texture2D& updOutputTexture();
        gl::FrameBuffer& updOutputFBO();

    private:
        class Impl;
        Impl* m_Impl;
    };
}