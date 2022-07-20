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
    struct ComponentDecoration;
    class ModelStateRendererParams;
}

namespace osc
{
    class ModelStateRenderer final {
    public:
        ModelStateRenderer();
        ModelStateRenderer(ModelStateRenderer const&) = delete;
        ModelStateRenderer(ModelStateRenderer&&) noexcept;
        ModelStateRenderer& operator=(ModelStateRenderer const&) = delete;
        ModelStateRenderer& operator=(ModelStateRenderer&&) noexcept;
        ~ModelStateRenderer() noexcept;

        glm::ivec2 getDimensions() const;
        int getSamples() const;
        void draw(nonstd::span<ComponentDecoration const>, ModelStateRendererParams const&);
        gl::Texture2D& updOutputTexture();
        gl::FrameBuffer& updOutputFBO();

    private:
        class Impl;
        Impl* m_Impl;
    };
}