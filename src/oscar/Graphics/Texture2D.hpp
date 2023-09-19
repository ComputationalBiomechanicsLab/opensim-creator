#pragma once

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Color32.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/TextureFilterMode.hpp>
#include <oscar/Graphics/TextureFormat.hpp>
#include <oscar/Graphics/TextureWrapMode.hpp>
#include <oscar/Utils/CopyOnUpdPtr.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <iosfwd>
#include <string_view>
#include <vector>

namespace osc { class Texture2D; }
namespace osc { struct Rect; }
namespace osc { void DrawTextureAsImGuiImage(Texture2D const&, glm::vec2, glm::vec2, glm::vec2); }
namespace osc { bool ImageButton(CStringView, Texture2D const&, glm::vec2, Rect const&); }

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // a handle to a 2D texture that can be rendered by the graphics backend
    class Texture2D final {
    public:
        Texture2D(
            glm::ivec2 dimensions,
            TextureFormat = TextureFormat::RGBA32,
            ColorSpace = ColorSpace::sRGB,
            TextureWrapMode = TextureWrapMode::Repeat,
            TextureFilterMode = TextureFilterMode::Linear
        );
        Texture2D(Texture2D const&);
        Texture2D(Texture2D&&) noexcept;
        Texture2D& operator=(Texture2D const&);
        Texture2D& operator=(Texture2D&&) noexcept;
        ~Texture2D() noexcept;

        glm::ivec2 getDimensions() const;
        TextureFormat getTextureFormat() const;
        ColorSpace getColorSpace() const;

        TextureWrapMode getWrapMode() const;  // same as getWrapModeU
        void setWrapMode(TextureWrapMode);  // sets all axes
        TextureWrapMode getWrapModeU() const;
        void setWrapModeU(TextureWrapMode);
        TextureWrapMode getWrapModeV() const;
        void setWrapModeV(TextureWrapMode);
        TextureWrapMode getWrapModeW() const;
        void setWrapModeW(TextureWrapMode);

        TextureFilterMode getFilterMode() const;
        void setFilterMode(TextureFilterMode);

        // - must contain pixels row-by-row
        // - the size of the span must equal the width*height of the texture
        // - may internally convert the provided `Color` structs into the format
        //   of the texture, so don't expect `getPixels` to necessarily return
        //   exactly the same values as provided
        std::vector<Color> getPixels() const;
        void setPixels(nonstd::span<Color const>);

        // - must contain pixels row-by-row
        // - the size of the span must equal the width*height of the texture
        // - may internally convert the provided `Color` structs into the format
        //   of the texture, so don't expect `getPixels` to necessarily return
        //   exactly the same values as provided
        std::vector<Color32> getPixels32() const;
        void setPixels32(nonstd::span<Color32 const>);

        // - must contain pixel _data_ row-by-row
        // - the size of the data span must be equal to:
        //     - width*height*NumBytesPerPixel(getTextureFormat())
        // - will not perform any internal conversion of the data (it's a memcpy)
        nonstd::span<uint8_t const> getPixelData() const;
        void setPixelData(nonstd::span<uint8_t const>);

        friend void swap(Texture2D& a, Texture2D& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

    private:
        friend void DrawTextureAsImGuiImage(Texture2D const&, glm::vec2, glm::vec2, glm::vec2);
        friend bool ImageButton(CStringView label, Texture2D const& t, glm::vec2, Rect const&);
        void* getTextureHandleHACK() const;  // used by ImGui... for now

        friend class GraphicsBackend;
        friend bool operator==(Texture2D const&, Texture2D const&) noexcept;
        friend bool operator!=(Texture2D const&, Texture2D const&) noexcept;
        friend std::ostream& operator<<(std::ostream&, Texture2D const&);

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    inline bool operator==(Texture2D const& a, Texture2D const& b) noexcept
    {
        return a.m_Impl == b.m_Impl;
    }

    inline bool operator!=(Texture2D const& a, Texture2D const& b) noexcept
    {
        return a.m_Impl != b.m_Impl;
    }

    std::ostream& operator<<(std::ostream&, Texture2D const&);
}
