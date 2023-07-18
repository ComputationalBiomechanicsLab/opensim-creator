#pragma once

#include "oscar/Graphics/RenderTextureDescriptor.hpp"

#include <memory>

namespace osc
{
    enum class RenderBufferType {
        Color = 0,
        Depth,
        TOTAL,
    };

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