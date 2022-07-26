#pragma once

#include "src/Graphics/ImageFlags.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <filesystem>

namespace osc
{
    class Image {
    public:
        static Image Load(std::filesystem::path const&, ImageFlags = ImageFlags_None);

        Image(Image const&) = delete;
        Image(Image&&) noexcept;
        Image& operator=(Image const&) = delete;
        Image& operator=(Image&&) noexcept;
        ~Image() noexcept;

        glm::ivec2 getDimensions() const;
        int getNumChannels() const;
        nonstd::span<uint8_t const> getPixelData() const;

    private:
        Image(std::filesystem::path const&, ImageFlags);

        glm::ivec2 m_Dimensions;
        int m_NumChannels;
        unsigned char* m_Pixels;
    };
}