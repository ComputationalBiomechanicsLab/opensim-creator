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
    inline constexpr int c_StbTrue = 1;
    inline constexpr int c_StbFalse = 0;

    // this mutex is required because stbi has global mutable state (e.g. stbi_set_flip_vertically_on_load)
    auto lockStbiAPI()
    {
        static std::mutex s_StbiMutex;
        return std::lock_guard{s_StbiMutex};
    }

    constexpr stbi_io_callbacks c_StbiIStreamCallbacks = {

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

    Texture2D load32BitTexture(
        std::istream& in,
        std::string_view name,
        ColorSpace colorSpace,
        ImageLoadingFlags flags)
    {
        const auto guard = lockStbiAPI();

        if (flags & ImageLoadingFlags::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_StbTrue);
        }

        Vec2i dims{};
        int numChannels = 0;
        const std::unique_ptr<float, decltype(&stbi_image_free)> pixels = {
            stbi_loadf_from_callbacks(&c_StbiIStreamCallbacks, &in, &dims.x, &dims.y, &numChannels, 0),
            stbi_image_free,
        };

        if (flags & ImageLoadingFlags::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_StbFalse);
        }

        if (!pixels) {
            std::stringstream ss;
            ss << name << ": error loading HDR image: " << stbi_failure_reason();
            throw std::runtime_error{std::move(ss).str()};
        }

        const std::optional<TextureFormat> format = ToTextureFormat(
            static_cast<size_t>(numChannels),
            TextureChannelFormat::Float32
        );

        if (!format) {
            std::stringstream ss;
            ss << name << ": error loading HDR image: no TextureFormat exists for " << numChannels << " floating-point channel images";
            throw std::runtime_error{std::move(ss).str()};
        }

        const std::span<const float> pixelSpan{
            pixels.get(),
            static_cast<size_t>(dims.x*dims.y*numChannels)
        };

        Texture2D rv{
            dims,
            *format,
            colorSpace,
        };
        rv.setPixelData(ViewObjectRepresentations<uint8_t>(pixelSpan));

        return rv;
    }

    Texture2D load8BitTexture(
        std::istream& in,
        std::string_view name,
        ColorSpace colorSpace,
        ImageLoadingFlags flags)
    {
        const auto guard = lockStbiAPI();

        if (flags & ImageLoadingFlags::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_StbTrue);
        }

        Vec2i dims{};
        int numChannels = 0;
        const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels = {
            stbi_load_from_callbacks(&c_StbiIStreamCallbacks, &in, &dims.x, &dims.y, &numChannels, 0),
            stbi_image_free,
        };

        if (flags & ImageLoadingFlags::FlipVertically) {
            stbi_set_flip_vertically_on_load(c_StbFalse);
        }

        if (!pixels) {
            std::stringstream ss;
            ss << name  << ": error loading non-HDR image: " << stbi_failure_reason();
            throw std::runtime_error{std::move(ss).str()};
        }

        const std::optional<TextureFormat> format = ToTextureFormat(
            static_cast<size_t>(numChannels),
            TextureChannelFormat::Uint8
        );

        if (!format) {
            std::stringstream ss;
            ss << name << ": error loading non-HDR image: no TextureFormat exists for " << numChannels << " 8-bit channel images";
            throw std::runtime_error{std::move(ss).str()};
        }

        Texture2D rv{
            dims,
            *format,
            colorSpace,
        };
        rv.setPixelData({pixels.get(), static_cast<size_t>(dims.x*dims.y*numChannels)});
        return rv;
    }

    void stbiWriteToStdOstream(void* context, void* data, int size)
    {
        std::ostream& out = *reinterpret_cast<std::ostream*>(context);
        if (size > 0) {
            out.write(std::launder(reinterpret_cast<const char*>(data)), std::streamsize{size});
        }
    }
}

Texture2D osc::loadTexture2DFromImage(
    std::istream& in,
    std::string_view name,
    ColorSpace colorSpace,
    ImageLoadingFlags flags)
{
    // test whether file content is HDR or not
    const auto originalPos = in.tellg();
    const bool isHDR = stbi_is_hdr_from_callbacks(&c_StbiIStreamCallbacks, &in) != 0;
    in.seekg(originalPos);  // rewind, before reading content

    OSC_ASSERT(in.tellg() == originalPos);

    return isHDR ?
        load32BitTexture(in, name, colorSpace, flags) :
        load8BitTexture(in, name, colorSpace, flags);
}

void osc::writeToPNG(
    const Texture2D& tex,
    std::ostream& out)
{
    const Vec2i dims = tex.getDimensions();
    const int stride = 4 * dims.x;
    const std::vector<Color32> pixels = tex.getPixels32();

    const auto guard = lockStbiAPI();

    stbi_flip_vertically_on_write(c_StbTrue);
    const int rv = stbi_write_png_to_func(
        stbiWriteToStdOstream,
        &out,
        dims.x,
        dims.y,
        4,
        pixels.data(),
        stride
    );
    stbi_flip_vertically_on_write(c_StbFalse);

    OSC_ASSERT(rv != 0);

    // (dtor drops guard here)
}
