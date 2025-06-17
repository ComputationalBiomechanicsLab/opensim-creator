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
    // Represents a 2D image array that can be read by `Shader`s.
    class Texture2D final {
    public:
        // Default constructs a single-pixel texture.
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
        // - These dimensions should be used when compositing the texture into a
        //   user interface.
        // - Returns the equivalent of `texture.dimensions() / texture.device_pixel_ratio()`.
        Vec2 device_independent_dimensions() const;

        // Returns the ratio of the resolution of the texture in physical pixels
        // to the resolution of it in device-independent pixels. This is useful
        // when compositing the texture into high DPI user interfaces that are built
        // around a device-independent pixel scaling architecture.
        float device_pixel_ratio() const;

        // Sets the device-to-pixel ratio for the texture, which has the effect
        // of scaling the `device_independent_dimensions()` of the texture.
        void set_device_pixel_ratio(float);

        // Returns the storage format of the underlying pixel data.
        TextureFormat texture_format() const;

        // Returns the color space of the texture.
        ColorSpace color_space() const;

        // Returns the equivalent of `wrap_mode_u`.
        TextureWrapMode wrap_mode() const;

        // Sets all wrap axes (`u`, `v`, and `w`) to the specified `TextureWrapMode`.
        void set_wrap_mode(TextureWrapMode);
        TextureWrapMode wrap_mode_u() const;
        void set_wrap_mode_u(TextureWrapMode);
        TextureWrapMode wrap_mode_v() const;
        void set_wrap_mode_v(TextureWrapMode);
        TextureWrapMode wrap_mode_w() const;
        void set_wrap_mode_w(TextureWrapMode);

        TextureFilterMode filter_mode() const;
        void set_filter_mode(TextureFilterMode);

        // Returns the pixels, parsed into a `Color` (i.e. HDR sRGB RGBA) format, where:
        //
        // - Pixels are returned row-by-row, where:
        //   - The first pixel corresponds to the lower-left corner of the image.
        //   - Subsequent pixels progress left-to-right through the remaining pixels in the
        //     lowest row of the image, and then in successively higher rows of the image.
        //   - The final pixel corresponds to the upper-right corner of the image.
        //   - Note: this right-handed coordinate system matches samplers in GLSL shaders. That
        //     is, a texture/uv coordinate of `(0, 0)` sampled in a shader would sample the
        //     bottom-left pixel of the texture in GLSL.
        // - The returned pixels are parsed from the underlying `TextureFormat` storage. If
        //   the storage format has fewer components than a `Color` (RGBA), the missing
        //   components default to `0.0f` - apart from alpha, which defaults to `1.0f`.
        std::vector<Color> pixels() const;

        // Assigns the given pixels to the texture.
        //
        // - Pixels should be provided row-by-row, where:
        //   - The first pixel corresponds to the lower-left corner of the image.
        //   - Subsequent pixels progress left-to-right through the remaining pixels in the
        //     lowest row of the image, and then in successively higher rows of the image.
        //   - The final pixel corresponds to the upper-right corner of the image.
        //   - Note: this right-handed coordinate system matches samplers in GLSL shaders. That
        //     is, a texture/uv coordinate of `(0, 0)` used in a shader would sample the bottom-left
        //     pixel of the texture in GLSL.
        // - The `size()` of the provided pixel span must be equal to the area of this texture.
        // - The provided pixels will be converted into the underlying `TextureFormat` storage
        //   of this texture, which may change the provided pixels' component values, depending
        //   on the format. This means that the return value of `pixels()` may not be equal to
        //   the pixels provided to this function.
        void set_pixels(std::span<const Color>);

        // Returns the pixels, parsed into a `Color32` (i.e. LDR sRGB RGBA) format, where:
        //
        // - Pixels are returned row-by-row
        //   - The first pixel corresponds to the lower-left corner of the image.
        //   - Subsequent pixels progress left-to-right through the remaining pixels in the
        //     lowest row of the image, and then in successively higher rows of the image.
        //   - The final pixel corresponds to the upper-right corner of the image.
        //   - Note: this right-handed coordinate system matches samplers in GLSL shaders. That
        //     is, a texture/uv coordinate of `(0, 0)` used in a shader would sample the bottom-left
        //     pixel of the texture in GLSL.
        // - The returned pixels are parsed from the underlying `TextureFormat` storage. If
        //   the storage format has fewer components than a `Color` (RGBA), the missing
        //   components default to `0.0f` - apart from alpha, which defaults to `1.0f`.
        std::vector<Color32> pixels32() const;
        void set_pixels32(std::span<const Color32>);

        // - must contain pixel _data_ row-by-row
        // - the size of the data span must be equal to:
        //     - width*height*NumBytesPerPixel(texture_format())
        // - will not perform any internal conversion of the data (it's a memcpy)
        std::span<const uint8_t> pixel_data() const;
        void set_pixel_data(std::span<const uint8_t>);

        friend bool operator==(const Texture2D&, const Texture2D&) = default;

        class Impl;

        // returns a reference to the `Texture2D`'s private implementation (for internal use).
        const Impl& impl() const { return *impl_; }
    private:
        friend std::ostream& operator<<(std::ostream&, const Texture2D&);
        friend class GraphicsBackend;

        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const Texture2D&);
}
