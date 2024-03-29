#pragma once

#include <oscar/Graphics/RenderBufferType.h>
#include <oscar/Graphics/RenderTextureDescriptor.h>

#include <memory>

namespace osc
{
    class RenderBuffer final {
    public:
        RenderBuffer(
            RenderTextureDescriptor const&,
            RenderBufferType
        );
        RenderBuffer(RenderBuffer const&) = delete;
        RenderBuffer(RenderBuffer&&) noexcept = delete;
        RenderBuffer& operator=(RenderBuffer const&) = delete;
        RenderBuffer& operator=(RenderBuffer&&) noexcept = delete;
        ~RenderBuffer() noexcept;

    private:
        friend class GraphicsBackend;
        friend class RenderTexture;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
