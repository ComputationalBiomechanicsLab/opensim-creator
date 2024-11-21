#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/ColorRenderBufferFormat.h>
#include <oscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <oscar/Graphics/RenderTextureParams.h>
#include <oscar/Graphics/SharedColorRenderBuffer.h>
#include <oscar/Graphics/SharedDepthStencilRenderBuffer.h>
#include <oscar/Graphics/TextureDimensionality.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <iosfwd>

namespace osc
{
    // render texture
    //
    // a texture that can be rendered to
    class RenderTexture final {
    public:
        RenderTexture();
        explicit RenderTexture(const RenderTextureParams&);

        Vec2i dimensions() const;
        void set_dimensions(Vec2i);

        TextureDimensionality dimensionality() const;
        void set_dimensionality(TextureDimensionality);

        ColorRenderBufferFormat color_format() const;
        void set_color_format(ColorRenderBufferFormat);

        AntiAliasingLevel anti_aliasing_level() const;
        void set_anti_aliasing_level(AntiAliasingLevel);

        DepthStencilRenderBufferFormat depth_stencil_format() const;
        void set_depth_stencil_format(DepthStencilRenderBufferFormat);

        void reformat(const RenderTextureParams&);

        SharedColorRenderBuffer upd_color_buffer();
        SharedDepthStencilRenderBuffer upd_depth_buffer();

        friend bool operator==(const RenderTexture&, const RenderTexture&) = default;

        friend std::ostream& operator<<(std::ostream&, const RenderTexture&);
    private:
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const RenderTexture&);
}
