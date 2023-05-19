#pragma once

#include "oscar/Graphics/RenderTextureFormat.hpp"
#include "oscar/Graphics/RenderTextureReadWrite.hpp"
#include "oscar/Graphics/DepthStencilFormat.hpp"

#include <glm/vec2.hpp>

#include <iosfwd>

namespace osc
{
    class RenderTextureDescriptor final {
    public:
        RenderTextureDescriptor(glm::ivec2 dimensions);
        RenderTextureDescriptor(RenderTextureDescriptor const&) = default;
        RenderTextureDescriptor(RenderTextureDescriptor&&) noexcept = default;
        RenderTextureDescriptor& operator=(RenderTextureDescriptor const&) = default;
        RenderTextureDescriptor& operator=(RenderTextureDescriptor&&) noexcept = default;
        ~RenderTextureDescriptor() noexcept = default;

        glm::ivec2 getDimensions() const;
        void setDimensions(glm::ivec2);

        int32_t getAntialiasingLevel() const;
        void setAntialiasingLevel(int32_t);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

        RenderTextureReadWrite getReadWrite() const;
        void setReadWrite(RenderTextureReadWrite);

    private:
        friend class GraphicsBackend;
        friend bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);

        glm::ivec2 m_Dimensions;
        int32_t m_AnialiasingLevel;
        RenderTextureFormat m_ColorFormat;
        DepthStencilFormat m_DepthStencilFormat;
        RenderTextureReadWrite m_ReadWrite;
    };

    bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);
}