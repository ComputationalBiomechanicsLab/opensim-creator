#pragma once

#include <liboscar/Graphics/CubemapFace.h>
#include <liboscar/Graphics/TextureFilterMode.h>
#include <liboscar/Graphics/TextureFormat.h>
#include <liboscar/Graphics/TextureWrapMode.h>
#include <liboscar/Utils/CopyOnUpdSharedValue.h>

#include <span>

namespace osc
{
    // Represents a single texture composed of six images that are tessellated into
    // a cube shape such that they can be sampled using a direction vector that
    // originates from the center of the cube (i.e. via a `samplerCube`, in GLSL).
    //
    // Note: Each of the six faces of the cube should be provided in the same way
    //       as for a `Texture2D` (i.e. starting in the bottom-left and moving row
    //       by row to the top right), but the direction vector in the GLSL shader
    //       is not in something resembling the texture or world coordinate system.
    //       Instead, it's in a left-handed cube map coordinate system that's used
    //       by shader implementations to figure out which of the six faces to
    //       address with a standard 2D vector in texture coordinate space.
    //
    //       See the OpenGL specification, section 8.13, "Cube Map Texture Selection"
    //       (e.g. https://registry.khronos.org/OpenGL/specs/gl/glspec46.core.pdf) for
    //       more details, but it usually means that the images either have to be
    //       rotated or the direction vector has to be flipped.
    class Cubemap final {
    public:
        // Constructs a cubemap that is `width` physical pixels wide and high, and
        // the given `format`.
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

        CopyOnUpdSharedValue<Impl> impl_;
    };
}
