#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/DepthStencilFormat.hpp>
#include <oscar/Graphics/RenderTextureFormat.hpp>
#include <oscar/Graphics/RenderTextureReadWrite.hpp>
#include <oscar/Graphics/TextureDimensionality.hpp>
#include <oscar/Maths/Vec2.hpp>

#include <iosfwd>

namespace osc
{
    class RenderTextureDescriptor final {
    public:
        RenderTextureDescriptor(Vec2i dimensions);
        RenderTextureDescriptor(RenderTextureDescriptor const&) = default;
        RenderTextureDescriptor(RenderTextureDescriptor&&) noexcept = default;
        RenderTextureDescriptor& operator=(RenderTextureDescriptor const&) = default;
        RenderTextureDescriptor& operator=(RenderTextureDescriptor&&) noexcept = default;
        ~RenderTextureDescriptor() noexcept = default;

        Vec2i getDimensions() const;
        void setDimensions(Vec2i);

        TextureDimensionality getDimensionality() const;
        void setDimensionality(TextureDimensionality);

        AntiAliasingLevel getAntialiasingLevel() const;
        void setAntialiasingLevel(AntiAliasingLevel);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

        RenderTextureReadWrite getReadWrite() const;
        void setReadWrite(RenderTextureReadWrite);

        friend bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&) = default;
    private:
        friend std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);
        friend class GraphicsBackend;

        Vec2i m_Dimensions;
        TextureDimensionality m_Dimension;
        AntiAliasingLevel m_AnialiasingLevel;
        RenderTextureFormat m_ColorFormat;
        DepthStencilFormat m_DepthStencilFormat;
        RenderTextureReadWrite m_ReadWrite;
    };

    std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);
}
