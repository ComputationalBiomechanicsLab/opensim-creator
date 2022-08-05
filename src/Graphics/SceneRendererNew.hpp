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
    class SceneRendererNewParams;
}

namespace osc
{
    class SceneRendererNew final {
    public:
        SceneRendererNew();
        SceneRendererNew(SceneRendererNew const&) = delete;
        SceneRendererNew(SceneRendererNew&&) noexcept;
        SceneRendererNew& operator=(SceneRendererNew const&) = delete;
        SceneRendererNew& operator=(SceneRendererNew&&) noexcept;
        ~SceneRendererNew() noexcept;

        glm::ivec2 getDimensions() const;
        int getSamples() const;
        void draw(nonstd::span<SceneDecorationNew const>, SceneRendererNewParams const&);
        gl::Texture2D& updOutputTexture();
        gl::FrameBuffer& updOutputFBO();

    private:
        class Impl;
        Impl* m_Impl;
    };
}