#pragma once

#include <liboscar/Graphics/CubemapFace.h>
#include <liboscar/Graphics/TextureFilterMode.h>
#include <liboscar/Graphics/TextureFormat.h>
#include <liboscar/Graphics/TextureWrapMode.h>
#include <liboscar/Utils/CopyOnUpdPtr.h>

#include <span>

namespace osc
{
    class Cubemap final {
    public:
        Cubemap(int32_t width, TextureFormat format);

        int32_t width() const;
        TextureFormat texture_format() const;

        TextureWrapMode wrap_mode() const;  // same as `wrap_mode_u`
        void set_wrap_mode(TextureWrapMode);  // sets all axes
        TextureWrapMode wrap_mode_u() const;
        void set_wrap_mode_u(TextureWrapMode);
        TextureWrapMode wrap_mode_v() const;
        void set_wrap_mode_v(TextureWrapMode);
        TextureWrapMode wrap_mode_w() const;
        void set_wrap_mode_w(TextureWrapMode);

        TextureFilterMode filter_mode() const;
        void set_filter_mode(TextureFilterMode);

        // The number of provided bytes must match the provided `width*width` and
        // `TextureFormat` of this `Cubemap`, or an exception will be thrown.
        void set_pixel_data(CubemapFace, std::span<const uint8_t>);

        friend bool operator==(const Cubemap&, const Cubemap&) = default;

        class Impl;
        const Impl& impl() const { return *impl_; }
    private:
        friend class GraphicsBackend;

        CopyOnUpdPtr<Impl> impl_;
    };
}
