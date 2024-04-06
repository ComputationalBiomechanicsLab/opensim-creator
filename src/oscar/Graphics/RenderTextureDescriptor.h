#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/DepthStencilFormat.h>
#include <oscar/Graphics/RenderTextureFormat.h>
#include <oscar/Graphics/RenderTextureReadWrite.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>

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

        Vec2i dimensions_;
        TextureDimensionality dimensionality_;
        AntiAliasingLevel antialiasing_level_;
        RenderTextureFormat color_format_;
        DepthStencilFormat depth_stencil_format_;
        RenderTextureReadWrite read_write_;
    };

    std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);
}
