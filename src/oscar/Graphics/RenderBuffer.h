#pragma once

#include <oscar/Graphics/RenderBufferType.h>

#include <memory>

namespace osc { struct RenderTextureParams; }

namespace osc
{
    class RenderBuffer final {
    public:
        RenderBuffer(
            const RenderTextureParams&,
            RenderBufferType
        );
        RenderBuffer(const RenderBuffer&);
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
