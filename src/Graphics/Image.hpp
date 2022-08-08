#pragma once

#include "src/Graphics/ImageFlags.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>

namespace osc
{
    class Image {
    public:
        static Image Load(std::filesystem::path const&, ImageFlags = ImageFlags_None);

        Image(glm::ivec2 dimensions, nonstd::span<uint8_t const> channelsRowByRow, int numChannels);
        Image(Image const&) = delete;
        Image(Image&&) noexcept;
        Image& operator=(Image const&) = delete;
        Image& operator=(Image&&) noexcept;
        ~Image() noexcept;

        glm::ivec2 getDimensions() const;
        int getNumChannels() const;
        nonstd::span<uint8_t const> getPixelData() const;

    private:
        glm::ivec2 m_Dimensions;
        int m_NumChannels;
        std::unique_ptr<uint8_t[]> m_Pixels;
    };
}