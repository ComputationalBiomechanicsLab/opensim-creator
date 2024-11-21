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

            std::istream& in = *static_cast<std::istream*>(user);
            in.read(data, size);
            return static_cast<int>(in.gcount());
        },

        .skip = [](void* user, int n) -> void
        {
            // stbi: skip the next 'n' bytes, or 'unget' the last -n bytes if negative

            std::istream& in = *static_cast<std::istream*>(user);
            in.seekg(n, std::ios::cur);
        },

        .eof = [](void* user) -> int
        {
            // stbi: returns nonzero if we are at end of file/data

            std::istream& in = *static_cast<std::istream*>(user);
            return in.eof() ? -1 : 0;
        }
    };

    Texture2D load_32bit_texture(
        std::istream& in,
        std::string_view input_name,
        ColorSpace color_space,
        ImageLoadingFlags flags)
    {
        const auto guard = lock_stbi_api();

        if (flags & ImageLoadingFlag::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_stb_true);
        }

        Vec2i dimensions{};
        int num_components = 0;
        const std::unique_ptr<float, decltype(&stbi_image_free)> pixel_data = {
            stbi_loadf_from_callbacks(&c_stbi_stream_callbacks, &in, &dimensions.x, &dimensions.y, &num_components, 0),
            stbi_image_free,
        };

        if (flags & ImageLoadingFlag::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_stb_false);
        }

        if (not pixel_data) {
            std::stringstream ss;
            ss << input_name << ": error loading HDR image: " << stbi_failure_reason();
            throw std::runtime_error{std::move(ss).str()};
        }

        const std::optional<TextureFormat> texture_format = to_texture_format(
            static_cast<size_t>(num_components),
            TextureComponentFormat::Float32
        );

        if (not texture_format) {
            std::stringstream ss;
            ss << input_name << ": error loading HDR image: no TextureFormat exists for " << num_components << "-floating-point component images";
            throw std::runtime_error{std::move(ss).str()};
        }

        const std::span<const float> pixel_span{
            pixel_data.get(),
            static_cast<size_t>(dimensions.x*dimensions.y*num_components)
        };

        Texture2D rv{dimensions, *texture_format, color_space};
        rv.set_pixel_data(view_object_representations<uint8_t>(pixel_span));
        return rv;
    }

    Texture2D load_8bit_texture(
        std::istream& in,
        std::string_view input_name,
        ColorSpace color_space,
        ImageLoadingFlags flags)
    {
        const auto guard = lock_stbi_api();

        if (flags & ImageLoadingFlag::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_stb_true);
        }

        Vec2i dimensions{};
        int num_components = 0;
        const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixel_data = {
            stbi_load_from_callbacks(&c_stbi_stream_callbacks, &in, &dimensions.x, &dimensions.y, &num_components, 0),
            stbi_image_free,
        };

        if (flags & ImageLoadingFlag::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_stb_false);
        }

        if (not pixel_data) {
            std::stringstream ss;
            ss << input_name  << ": error loading non-HDR image: " << stbi_failure_reason();
            throw std::runtime_error{std::move(ss).str()};
        }

        const std::optional<TextureFormat> texture_format = to_texture_format(
            static_cast<size_t>(num_components),
            TextureComponentFormat::Uint8
        );

        if (not texture_format) {
            std::stringstream ss;
            ss << input_name << ": error loading non-HDR image: no TextureFormat exists for " << num_components << "-8-bit component images";
            throw std::runtime_error{std::move(ss).str()};
        }

        Texture2D rv{dimensions, *texture_format, color_space};
        rv.set_pixel_data({pixel_data.get(), static_cast<size_t>(dimensions.x*dimensions.y*num_components)});
        return rv;
    }

    void osc_stbi_write_via_std_ostream(void* context, void* data, int size)
    {
        std::ostream& out = *static_cast<std::ostream*>(context);
        if (size > 0) {
            out.write(static_cast<const char*>(data), std::streamsize{size});
        }
    }
}

Texture2D osc::load_texture2D_from_image(
    std::istream& in,
    std::string_view input_name,
    ColorSpace color_space,
    ImageLoadingFlags flags)
{
    // test whether file content is HDR or not
    const auto original_cursor_pos = in.tellg();
    const bool is_hdr = stbi_is_hdr_from_callbacks(&c_stbi_stream_callbacks, &in) != 0;
    in.seekg(original_cursor_pos);  // rewind, before reading content

    OSC_ASSERT_ALWAYS(in.tellg() == original_cursor_pos && "could not rewind the stream (required for loading images)");

    return is_hdr ?
        load_32bit_texture(in, input_name, color_space, flags) :
        load_8bit_texture(in, input_name, color_space, flags);
}

void osc::write_to_png(
    const Texture2D& texture,
    std::ostream& out)
{
    const Vec2i dimensions = texture.dimensions();
    const int row_stride = 4 * dimensions.x;
    const std::vector<Color32> pixels = texture.pixels32();

    const auto guard = lock_stbi_api();

    stbi_flip_vertically_on_write(c_stb_true);
    const int rv = stbi_write_png_to_func(
        osc_stbi_write_via_std_ostream,
        &out,
        dimensions.x,
        dimensions.y,
        4,
        pixels.data(),
        row_stride
    );
    stbi_flip_vertically_on_write(c_stb_false);

    if (rv == 0) {
        std::stringstream ss;
        ss << "failed to write a texture as a PNG: " << stbi_failure_reason();
        throw std::runtime_error{std::move(ss).str()};
    }
}
