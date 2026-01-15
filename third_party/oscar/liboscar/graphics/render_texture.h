#pragma once

#include <liboscar/graphics/anti_aliasing_level.h>
#include <liboscar/graphics/color_render_buffer_format.h>
#include <liboscar/graphics/depth_stencil_render_buffer_format.h>
#include <liboscar/graphics/render_texture_params.h>
#include <liboscar/graphics/shared_color_render_buffer.h>
#include <liboscar/graphics/shared_depth_stencil_render_buffer.h>
#include <liboscar/graphics/texture_dimensionality.h>
#include <liboscar/maths/vector2.h>
#include <liboscar/utils/copy_on_upd_shared_value.h>

#include <iosfwd>

namespace osc
{
    // A texture that can receive the result of a render pass.
    class RenderTexture final {
    public:
        RenderTexture();
        explicit RenderTexture(const RenderTextureParams&);

        // Returns the dimensions of the texture in physical pixels.
        Vector2i pixel_dimensions() const;

        // Sets the dimensions of the texture in physical pixels.
        void set_pixel_dimensions(Vector2i);

        // Returns the dimensions of the texture in device-independent pixels.
        //
        // The return value is equivalent to `texture.pixel_dimensions() / texture.device_pixel_ratio()`.
        Vector2 dimensions() const;

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

        CopyOnUpdSharedValue<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const RenderTexture&);
}
