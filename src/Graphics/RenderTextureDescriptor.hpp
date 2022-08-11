#pragma once

#include "src/Graphics/RenderTextureFormat.hpp"
#include "src/Graphics/DepthStencilFormat.hpp"

#include <iosfwd>

namespace osc
{
    class RenderTextureDescriptor final {
    public:
        RenderTextureDescriptor(int width, int height);
        RenderTextureDescriptor(RenderTextureDescriptor const&);
        RenderTextureDescriptor(RenderTextureDescriptor&&) noexcept;
        RenderTextureDescriptor& operator=(RenderTextureDescriptor const&);
        RenderTextureDescriptor& operator=(RenderTextureDescriptor&&) noexcept;
        ~RenderTextureDescriptor() noexcept;

        int getWidth() const;
        void setWidth(int);

        int getHeight() const;
        void setHeight(int);

        int getAntialiasingLevel() const;
        void setAntialiasingLevel(int);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

    private:
        friend class GraphicsBackend;
        friend bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator<(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);

        int m_Width;
        int m_Height;
        int m_AnialiasingLevel;
        RenderTextureFormat m_ColorFormat;
        DepthStencilFormat m_DepthStencilFormat;
    };

    bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator<(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);
}