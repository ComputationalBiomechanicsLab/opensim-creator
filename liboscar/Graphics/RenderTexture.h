#pragma once

#include <liboscar/Graphics/AntiAliasingLevel.h>
#include <liboscar/Graphics/ColorRenderBufferFormat.h>
#include <liboscar/Graphics/DepthStencilRenderBufferFormat.h>
#include <liboscar/Graphics/RenderTextureParams.h>
#include <liboscar/Graphics/SharedColorRenderBuffer.h>
#include <liboscar/Graphics/SharedDepthStencilRenderBuffer.h>
#include <liboscar/Graphics/TextureDimensionality.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Utils/CopyOnUpdPtr.h>

#include <iosfwd>

namespace osc
{
    // A texture that can receive the result of a render pass.
    class RenderTexture final {
    public:
        RenderTexture();
        explicit RenderTexture(const RenderTextureParams&);

        // Returns the dimensions of the texture in physical pixels.
        Vec2i pixel_dimensions() const;

        // Sets the dimensions of the texture in physical pixels.
        void set_pixel_dimensions(Vec2i);

        // Returns the dimensions of the texture in device-independent pixels.
        //
        // The return value is equivalent to `texture.pixel_dimensions() / texture.device_pixel_ratio()`.
        Vec2 dimensions() const;

        // Returns the ratio of the resolution of the texture in physical pixels
        // to the resolution of it in device-independent pixels.
        float device_pixel_ratio() const;

        // Sets the device-to-pixel ratio for the texture, which has the effect
        // of scaling the `device_independent_dimensions()` of the texture.
        void set_device_pixel_ratio(float);

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

        class Impl;
        const Impl& impl() const { return *impl_; }
    private:
        friend class GraphicsBackend;

        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const RenderTexture&);
}
