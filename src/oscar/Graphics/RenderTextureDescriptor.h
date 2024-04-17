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
        RenderTextureDescriptor(const RenderTextureDescriptor&) = default;
        RenderTextureDescriptor(RenderTextureDescriptor&&) noexcept = default;
        RenderTextureDescriptor& operator=(const RenderTextureDescriptor&) = default;
        RenderTextureDescriptor& operator=(RenderTextureDescriptor&&) noexcept = default;
        ~RenderTextureDescriptor() noexcept = default;

        Vec2i dimensions() const;
        void set_dimensions(Vec2i);

        TextureDimensionality dimensionality() const;
        void set_dimensionality(TextureDimensionality);

        AntiAliasingLevel anti_aliasing_level() const;
        void set_anti_aliasing_level(AntiAliasingLevel);

        RenderTextureFormat color_format() const;
        void set_color_format(RenderTextureFormat);

        DepthStencilFormat depth_stencil_format() const;
        void set_depth_stencil_format(DepthStencilFormat);

        RenderTextureReadWrite read_write() const;
        void set_read_write(RenderTextureReadWrite);

        friend bool operator==(const RenderTextureDescriptor&, const RenderTextureDescriptor&) = default;
    private:
        friend std::ostream& operator<<(std::ostream&, const RenderTextureDescriptor&);
        friend class GraphicsBackend;

        Vec2i dimensions_;
        TextureDimensionality dimensionality_;
        AntiAliasingLevel antialiasing_level_;
        RenderTextureFormat color_format_;
        DepthStencilFormat depth_stencil_format_;
        RenderTextureReadWrite read_write_;
    };

    std::ostream& operator<<(std::ostream&, const RenderTextureDescriptor&);
}
