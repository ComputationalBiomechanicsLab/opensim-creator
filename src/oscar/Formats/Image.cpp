#include "Image.h"

#include <oscar/Graphics/Color32.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/ObjectRepresentation.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <istream>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>

using namespace osc;

namespace
{
    inline constexpr int c_stb_true = 1;
    inline constexpr int c_stb_false = 0;

    // this mutex is required because stbi has global mutable state (e.g. `stbi_set_flip_vertically_on_load`)
    auto lock_stbi_api()
    {
        static std::mutex s_stbi_mutex;
        return std::lock_guard{s_stbi_mutex};
    }

    // OSC-specific IO callbacks for `stbi`, to make it compatible with (e.g.) virtual filesystems
    //
    // assumes the caller knows to set the user data `void*` ptr to an `std::istream`
    constexpr stbi_io_callbacks c_stbi_stream_callbacks = {

        .read = [](void* user, char* data, int size) -> int
        {
            // stbi: fill 'data' with 'size' bytes.  return number of bytes actually read

            std::istream& in = *reinterpret_cast<std::istream*>(user);
            in.read(data, size);
            return static_cast<int>(in.gcount());
        },

        .skip = [](void* user, int n) -> void
        {
            // stbi: skip the next 'n' bytes, or 'unget' the last -n bytes if negative

            std::istream& in = *reinterpret_cast<std::istream*>(user);
            in.seekg(n, std::ios::cur);
        },

        .eof = [](void* user) -> int
        {
            // stbi: returns nonzero if we are at end of file/data

            std::istream& in = *reinterpret_cast<std::istream*>(user);
            return in.eof() ? -1 : 0;
        }
    };

    Texture2D load_32bit_texture(
        std::istream& in,
        std::string_view name,
        ColorSpace color_space,
        ImageLoadingFlags flags)
    {
        const auto guard = lock_stbi_api();

        if (flags & ImageLoadingFlags::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_stb_true);
        }

        Vec2i dims{};
        int num_channels = 0;
        const std::unique_ptr<float, decltype(&stbi_image_free)> pixels = {
            stbi_loadf_from_callbacks(&c_stbi_stream_callbacks, &in, &dims.x, &dims.y, &num_channels, 0),
            stbi_image_free,
        };

        if (flags & ImageLoadingFlags::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_stb_false);
        }

        if (not pixels) {
            std::stringstream ss;
            ss << name << ": error loading HDR image: " << stbi_failure_reason();
            throw std::runtime_error{std::move(ss).str()};
        }

        const std::optional<TextureFormat> format = ToTextureFormat(
            static_cast<size_t>(num_channels),
            TextureChannelFormat::Float32
        );

        if (not format) {
            std::stringstream ss;
            ss << name << ": error loading HDR image: no TextureFormat exists for " << num_channels << " floating-point channel images";
            throw std::runtime_error{std::move(ss).str()};
        }

        const std::span<const float> pixel_span{
            pixels.get(),
            static_cast<size_t>(dims.x*dims.y*num_channels)
        };

        Texture2D rv{dims, *format, color_space};
        rv.set_pixel_data(view_object_representations<uint8_t>(pixel_span));
        return rv;
    }

    Texture2D load_8bit_texture(
        std::istream& in,
        std::string_view name,
        ColorSpace color_space,
        ImageLoadingFlags flags)
    {
        const auto guard = lock_stbi_api();

        if (flags & ImageLoadingFlags::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_stb_true);
        }

        Vec2i dims{};
        int num_channels = 0;
        const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels = {
            stbi_load_from_callbacks(&c_stbi_stream_callbacks, &in, &dims.x, &dims.y, &num_channels, 0),
            stbi_image_free,
        };

        if (flags & ImageLoadingFlags::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_stb_false);
        }

        if (not pixels) {
            std::stringstream ss;
            ss << name  << ": error loading non-HDR image: " << stbi_failure_reason();
            throw std::runtime_error{std::move(ss).str()};
        }

        const std::optional<TextureFormat> format = ToTextureFormat(
            static_cast<size_t>(num_channels),
            TextureChannelFormat::Uint8
        );

        if (not format) {
            std::stringstream ss;
            ss << name << ": error loading non-HDR image: no TextureFormat exists for " << num_channels << " 8-bit channel images";
            throw std::runtime_error{std::move(ss).str()};
        }

        Texture2D rv{dims, *format, color_space};
        rv.set_pixel_data({pixels.get(), static_cast<size_t>(dims.x*dims.y*num_channels)});
        return rv;
    }

    void osc_stbi_write_via_std_ostream(void* context, void* data, int size)
    {
        std::ostream& out = *reinterpret_cast<std::ostream*>(context);
        if (size > 0) {
            out.write(std::launder(reinterpret_cast<const char*>(data)), std::streamsize{size});
        }
    }
}

Texture2D osc::load_texture2D_from_image(
    std::istream& in,
    std::string_view name,
    ColorSpace color_space,
    ImageLoadingFlags flags)
{
    // test whether file content is HDR or not
    const auto original_cursor_pos = in.tellg();
    const bool is_hdr = stbi_is_hdr_from_callbacks(&c_stbi_stream_callbacks, &in) != 0;
    in.seekg(original_cursor_pos);  // rewind, before reading content

    OSC_ASSERT(in.tellg() == original_cursor_pos);

    return is_hdr ?
        load_32bit_texture(in, name, color_space, flags) :
        load_8bit_texture(in, name, color_space, flags);
}

void osc::write_to_png(
    const Texture2D& texture,
    std::ostream& out)
{
    const Vec2i dims = texture.getDimensions();
    const int stride = 4 * dims.x;
    const std::vector<Color32> pixels = texture.getPixels32();

    const auto guard = lock_stbi_api();

    stbi_flip_vertically_on_write(c_stb_true);
    const int rv = stbi_write_png_to_func(
        osc_stbi_write_via_std_ostream,
        &out,
        dims.x,
        dims.y,
        4,
        pixels.data(),
        stride
    );
    stbi_flip_vertically_on_write(c_stb_false);

    OSC_ASSERT(rv != 0);
}
