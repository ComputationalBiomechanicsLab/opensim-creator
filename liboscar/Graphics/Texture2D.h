#pragma once

#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/Color32.h>
#include <liboscar/Graphics/ColorSpace.h>
#include <liboscar/Graphics/TextureFilterMode.h>
#include <liboscar/Graphics/TextureFormat.h>
#include <liboscar/Graphics/TextureWrapMode.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Utils/CopyOnUpdPtr.h>

#include <cstdint>
#include <iosfwd>
#include <span>
#include <vector>

namespace osc
{
    // A 2D texture that can be rendered by the graphics backend.
    class Texture2D final {
    public:
        // Default-constructs a single-pixel texture as a placeholder
        Texture2D() :
            Texture2D{Vec2i{1, 1}}
        {}

        explicit Texture2D(
            Vec2i dimensions,
            TextureFormat = TextureFormat::RGBA32,
            ColorSpace = ColorSpace::sRGB,
            TextureWrapMode = TextureWrapMode::Repeat,
            TextureFilterMode = TextureFilterMode::Linear
        );

        // Returns the dimensions of the texture in physical pixels.
        Vec2i dimensions() const;

        // Returns the dimensions of the texture in device-independent pixels.
        //
        // These dimensions should be used when compositing the texture in a
        // user interface.
        //
        // The return value is equivalent to `texture.dimensions() / texture.device_pixel_ratio()`.
        Vec2 device_independent_dimensions() const;

        // Returns the ratio of the resolution of the texture in physical pixels
        // to the resolution of it in device-independent pixels.
        float device_pixel_ratio() const;

        // Sets the device-to-pixel ratio for the texture, which has the effect
        // of scaling the `device_independent_dimensions()` of the texture.
        void set_device_pixel_ratio(float);

        // Returns the format of the underlying pixel data.
        TextureFormat texture_format() const;

        // Returns the color space of the texture.
        ColorSpace color_space() const;

        TextureWrapMode wrap_mode() const;  // same as wrap_mode_u
        void set_wrap_mode(TextureWrapMode);  // sets all axes
        TextureWrapMode wrap_mode_u() const;
        void set_wrap_mode_u(TextureWrapMode);
        TextureWrapMode wrap_mode_v() const;
        void set_wrap_mode_v(TextureWrapMode);
        TextureWrapMode wrap_mode_w() const;
        void set_wrap_mode_w(TextureWrapMode);

        TextureFilterMode filter_mode() const;
        void set_filter_mode(TextureFilterMode);

        // - must contain pixels row-by-row
        // - the size of the span must equal the width*height of the texture
        // - may internally convert the provided `Color` structs into the format
        //   of the texture, so don't expect `pixels` to necessarily return
        //   exactly the same values as provided
        std::vector<Color> pixels() const;
        void set_pixels(std::span<const Color>);

        // - must contain pixels row-by-row
        // - the size of the span must equal the width*height of the texture
        // - may internally convert the provided `Color` structs into the format
        //   of the texture, so don't expect `pixels` to necessarily return
        //   exactly the same values as provided
        std::vector<Color32> pixels32() const;
        void set_pixels32(std::span<const Color32>);

        // - must contain pixel _data_ row-by-row
        // - the size of the data span must be equal to:
        //     - width*height*NumBytesPerPixel(texture_format())
        // - will not perform any internal conversion of the data (it's a memcpy)
        std::span<const uint8_t> pixel_data() const;
        void set_pixel_data(std::span<const uint8_t>);

        friend bool operator==(const Texture2D&, const Texture2D&) = default;

    private:
        friend std::ostream& operator<<(std::ostream&, const Texture2D&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const Texture2D&);
}
