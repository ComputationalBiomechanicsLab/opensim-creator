#pragma once

#include "src/Graphics/RenderTextureFormat.hpp"
#include "src/Graphics/DepthStencilFormat.hpp"

#include <glm/vec2.hpp>

#include <iosfwd>

namespace osc
{
    class RenderTextureDescriptor final {
    public:
        RenderTextureDescriptor(glm::ivec2 dimensions);
        RenderTextureDescriptor(RenderTextureDescriptor const&);
        RenderTextureDescriptor(RenderTextureDescriptor&&) noexcept;
        RenderTextureDescriptor& operator=(RenderTextureDescriptor const&);
        RenderTextureDescriptor& operator=(RenderTextureDescriptor&&) noexcept;
        ~RenderTextureDescriptor() noexcept;

        glm::ivec2 getDimensions() const;
        void setDimensions(glm::ivec2);

        int32_t getAntialiasingLevel() const;
        void setAntialiasingLevel(int32_t);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

    private:
        friend class GraphicsBackend;
        friend bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);

        glm::ivec2 m_Dimensions;
        int32_t m_AnialiasingLevel;
        RenderTextureFormat m_ColorFormat;
        DepthStencilFormat m_DepthStencilFormat;
    };

    bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);
}