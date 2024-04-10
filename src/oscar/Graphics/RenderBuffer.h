#pragma once

#include <oscar/Graphics/RenderBufferType.h>
#include <oscar/Graphics/RenderTextureDescriptor.h>

#include <memory>

namespace osc
{
    class RenderBuffer final {
    public:
        RenderBuffer(
            const RenderTextureDescriptor&,
            RenderBufferType
        );
        RenderBuffer(const RenderBuffer&) = delete;
        RenderBuffer(RenderBuffer&&) noexcept = delete;
        RenderBuffer& operator=(const RenderBuffer&) = delete;
        RenderBuffer& operator=(RenderBuffer&&) noexcept = delete;
        ~RenderBuffer() noexcept;

    private:
        friend class GraphicsBackend;
        friend class RenderTexture;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
