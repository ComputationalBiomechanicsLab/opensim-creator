#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Color32.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/TextureFilterMode.h>
#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Graphics/TextureWrapMode.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <cstdint>
#include <iosfwd>
#include <span>
#include <vector>

namespace osc
{
    // a handle to a 2D texture that can be rendered by the graphics backend
    class Texture2D final {
    public:
        explicit Texture2D(
            Vec2i dimensions,
            TextureFormat = TextureFormat::RGBA32,
            ColorSpace = ColorSpace::sRGB,
            TextureWrapMode = TextureWrapMode::Repeat,
            TextureFilterMode = TextureFilterMode::Linear
        );

        Vec2i dimensions() const;
        TextureFormat texture_format() const;
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
