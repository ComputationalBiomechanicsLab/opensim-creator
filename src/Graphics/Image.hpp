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
        Image();
        Image(
            glm::ivec2 dimensions,
            nonstd::span<uint8_t const> channelsRowByRow,
            int32_t numChannels
        );
        Image(Image const&);
        Image(Image&&) noexcept;
        Image& operator=(Image const&);
        Image& operator=(Image&&) noexcept;
        ~Image() noexcept;

        glm::ivec2 getDimensions() const;
        int32_t getNumChannels() const;
        nonstd::span<uint8_t const> getPixelData() const;

    private:
        glm::ivec2 m_Dimensions;
        int32_t m_NumChannels;
        std::unique_ptr<uint8_t[]> m_Pixels;
    };

    Image LoadImage(std::filesystem::path const&, ImageFlags = ImageFlags_None);
    void WriteToPNG(Image const&, std::filesystem::path const&);
}