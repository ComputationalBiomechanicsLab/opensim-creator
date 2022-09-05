#include "Image.hpp"

#include "src/Utils/Assertions.hpp"
#include "src/Utils/ScopeGuard.hpp"

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
#include <sstream>
#include <string>
#include <stdexcept>
#include <utility>


osc::Image osc::Image::Load(std::filesystem::path const& p, ImageFlags flags)
{
    if (flags & ImageFlags_FlipVertically)
    {
        stbi_set_flip_vertically_on_load(true);
    }

    glm::ivec2 dims = {0, 0};
    int channels = 0;
    stbi_uc* pixels = stbi_load(p.string().c_str(), &dims.x, &dims.y, &channels, 0);
    OSC_SCOPE_GUARD_IF(pixels, { stbi_image_free(pixels); });

    if (flags & ImageFlags_FlipVertically)
    {
        stbi_set_flip_vertically_on_load(false);
    }

    if (!pixels)
    {
        std::stringstream ss;
        ss << p << ": error loading image path: " << stbi_failure_reason();
        throw std::runtime_error{std::move(ss).str()};
    }

    nonstd::span<uint8_t> dataSpan{pixels, static_cast<size_t>(dims.x * dims.y * channels)};

    return Image{dims, dataSpan, channels};
}

osc::Image::Image(glm::ivec2 dimensions, nonstd::span<uint8_t const> channelsRowByRow, int numChannels) :
    m_Dimensions{std::move(dimensions)},
    m_NumChannels{std::move(numChannels)},
    m_Pixels{new uint8_t[dimensions.x * dimensions.y * numChannels]}
{
    std::copy(channelsRowByRow.begin(), channelsRowByRow.end(), m_Pixels.get());
}

osc::Image::Image(Image const& other) :
    m_Dimensions{other.m_Dimensions},
    m_NumChannels{other.m_NumChannels},
    m_Pixels{new uint8_t[m_Dimensions.x * m_Dimensions.y * m_NumChannels]}
{
    std::copy(other.m_Pixels.get(), other.m_Pixels.get() + (m_Dimensions.x*m_Dimensions.y*m_NumChannels), m_Pixels.get());
}

osc::Image::Image(Image&&) noexcept = default;
osc::Image& osc::Image::operator=(Image&& tmp) noexcept = default;
osc::Image::~Image() noexcept = default;

glm::ivec2 osc::Image::getDimensions() const
{
    return m_Dimensions;
}

int osc::Image::getNumChannels() const
{
    return m_NumChannels;
}

nonstd::span<uint8_t const> osc::Image::getPixelData() const
{
    return {m_Pixels.get(), static_cast<std::size_t>(m_Dimensions.x * m_Dimensions.y * m_NumChannels)};
}

void osc::WriteToPNG(Image const& image, std::filesystem::path const& outpath)
{
    std::string const pathStr = outpath.string();
    int const w = image.getDimensions().x;
    int const h = image.getDimensions().y;
    int const strideBetweenRows = w * image.getNumChannels();


    stbi_flip_vertically_on_write(true);
    int const rv = stbi_write_png(pathStr.c_str(), w, h, image.getNumChannels(), image.getPixelData().data(), strideBetweenRows);
    stbi_flip_vertically_on_write(false);

    OSC_ASSERT(rv != 0);
}