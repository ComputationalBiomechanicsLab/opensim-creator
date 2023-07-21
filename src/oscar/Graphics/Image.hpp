#pragma once

#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/ImageLoadingFlags.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>

namespace osc
{
    class Image final {
    public:
        Image();
        Image(
            glm::ivec2 dimensions,
            nonstd::span<uint8_t const> channelsRowByRow,
            int32_t numChannels,
            ColorSpace
        );
        Image(Image const&);
        Image(Image&&) noexcept;
        Image& operator=(Image const&);
        Image& operator=(Image&&) noexcept;
        ~Image() noexcept;

        glm::ivec2 getDimensions() const;
        int32_t getNumChannels() const;
        nonstd::span<uint8_t const> getPixelData() const;
        ColorSpace getColorSpace() const;

    private:
        glm::ivec2 m_Dimensions;
        int32_t m_NumChannels;
        std::unique_ptr<uint8_t[]> m_Pixels;
        ColorSpace m_ColorSpace;
    };

    Image LoadImageFromFile(
        std::filesystem::path const&,
        ColorSpace colorSpace,
        ImageLoadingFlags = ImageLoadingFlags::None
    );

    void WriteImageToPNGFile(
        Image const&,
        std::filesystem::path const&
    );
}
