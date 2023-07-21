#include "Image.hpp"

#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Utils/Assertions.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <stdexcept>
#include <utility>

namespace
{
    // this mutex is required because stbi has global mutable state (e.g. stbi_set_flip_vertically_on_load)
    auto LockStbiAPI()
    {
        static std::mutex s_StbiMutex;
        return std::lock_guard{s_StbiMutex};
    }
}

osc::Image::Image() :
    m_Dimensions{0, 0},
    m_NumChannels{4},
    m_Pixels{nullptr},
    m_ColorSpace{ColorSpace::sRGB}
{
}

osc::Image::Image(
    glm::ivec2 dimensions,
    nonstd::span<uint8_t const> channelsRowByRow,
    int32_t numChannels,
    ColorSpace colorSpace) :

    m_Dimensions{dimensions},
    m_NumChannels{numChannels},
    m_Pixels{new uint8_t[dimensions.x * dimensions.y * numChannels]},
    m_ColorSpace{colorSpace}
{
    std::copy(channelsRowByRow.begin(), channelsRowByRow.end(), m_Pixels.get());
}

osc::Image::Image(Image const& other) :
    m_Dimensions{other.m_Dimensions},
    m_NumChannels{other.m_NumChannels},
    m_Pixels{new uint8_t[m_Dimensions.x * m_Dimensions.y * m_NumChannels]},
    m_ColorSpace{other.m_ColorSpace}
{
    std::copy(
        other.m_Pixels.get(),
        other.m_Pixels.get() + (m_Dimensions.x*m_Dimensions.y*m_NumChannels),
        m_Pixels.get()
    );
}

osc::Image& osc::Image::operator=(Image const& other)
{
    Image copy{other};
    std::swap(*this, copy);
    return *this;
}

osc::Image::Image(Image&&) noexcept = default;
osc::Image& osc::Image::operator=(Image&&) noexcept = default;
osc::Image::~Image() noexcept = default;

glm::ivec2 osc::Image::getDimensions() const
{
    return m_Dimensions;
}

int32_t osc::Image::getNumChannels() const
{
    return m_NumChannels;
}

nonstd::span<uint8_t const> osc::Image::getPixelData() const
{
    return {m_Pixels.get(), static_cast<std::size_t>(m_Dimensions.x * m_Dimensions.y * m_NumChannels)};
}

osc::ColorSpace osc::Image::getColorSpace() const
{
    return m_ColorSpace;
}

osc::Image osc::LoadImageFromFile(
    std::filesystem::path const& p,
    ColorSpace colorSpace,
    ImageLoadingFlags flags)
{
    auto guard = LockStbiAPI();

    if (flags & ImageLoadingFlags::FlipVertically)
    {
        stbi_set_flip_vertically_on_load(true);
    }

    glm::ivec2 dims = {0, 0};
    int32_t channels = 0;
    std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels =
    {
        stbi_load(p.string().c_str(), &dims.x, &dims.y, &channels, 0),
        stbi_image_free,
    };

    if (flags & ImageLoadingFlags::FlipVertically)
    {
        stbi_set_flip_vertically_on_load(false);
    }

    if (!pixels)
    {
        std::stringstream ss;
        ss << p << ": error loading image path: " << stbi_failure_reason();
        throw std::runtime_error{std::move(ss).str()};
    }

    nonstd::span<uint8_t> dataSpan{pixels.get(), static_cast<size_t>(dims.x * dims.y * channels)};

    return Image
    {
        dims,
        dataSpan,
        channels,
        colorSpace,
    };
}

void osc::WriteImageToPNGFile(
    Image const& image,
    std::filesystem::path const& outpath)
{
    std::string const pathStr = outpath.string();
    glm::ivec2 dims = image.getDimensions();
    int32_t const strideBetweenRows = dims.x * image.getNumChannels();

    auto guard = LockStbiAPI();
    stbi_flip_vertically_on_write(true);
    auto const rv = stbi_write_png(
        pathStr.c_str(),
        dims.x,
        dims.y,
        image.getNumChannels(),
        image.getPixelData().data(),
        strideBetweenRows
    );
    stbi_flip_vertically_on_write(false);

    OSC_ASSERT(rv != 0);
}
