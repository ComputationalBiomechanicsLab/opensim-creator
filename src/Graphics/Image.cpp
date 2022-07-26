#include "Image.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string>
#include <stdexcept>
#include <utility>


osc::Image osc::Image::Load(std::filesystem::path const& p, ImageFlags flags)
{
    return Image{p, std::move(flags)};
}

osc::Image::Image(std::filesystem::path const& path, ImageFlags flags)
{
    if (flags & ImageFlags_FlipVertically)
    {
        stbi_set_flip_vertically_on_load(true);
    }

    m_Pixels = stbi_load(path.string().c_str(), &m_Dimensions.x, &m_Dimensions.y, &m_NumChannels, 0);

    if (flags & ImageFlags_FlipVertically)
    {
        stbi_set_flip_vertically_on_load(false);
    }

    if (!m_Pixels)
    {
        std::stringstream ss;
        ss << path << ": error loading image path: " << stbi_failure_reason();
        throw std::runtime_error{std::move(ss).str()};
    }
}

osc::Image::Image(Image&& tmp) noexcept : 
    m_Dimensions{std::move(tmp.m_Dimensions)},
    m_NumChannels{std::move(tmp.m_NumChannels)},
    m_Pixels{std::exchange(tmp.m_Pixels, nullptr)}
{
}

osc::Image& osc::Image::operator=(Image&& tmp) noexcept
{
    std::swap(m_Dimensions, tmp.m_Dimensions);
    std::swap(m_NumChannels, tmp.m_NumChannels);
    std::swap(m_Pixels, tmp.m_Pixels);
    return *this;
}

osc::Image::~Image() noexcept
{
    if (m_Pixels)
    {
        stbi_image_free(m_Pixels);
    }
}

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
    return {m_Pixels, static_cast<std::size_t>(m_Dimensions.x * m_Dimensions.y * m_NumChannels)};
}
