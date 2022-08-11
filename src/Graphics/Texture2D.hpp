#pragma once

#include "src/Graphics/Color.hpp"
#include "src/Graphics/ImageFlags.hpp"
#include "src/Graphics/TextureFilterMode.hpp"
#include "src/Graphics/TextureWrapMode.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string_view>

namespace osc
{
    struct Rgba32;
    class Texture2D;
    void DrawTextureAsImGuiImage(Texture2D&, glm::vec2);
}

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // a handle to a 2D texture that can be rendered by the graphics backend
    class Texture2D final {
    public:
        Texture2D(int width, int height, nonstd::span<Rgba32 const> rgbaPixelsRowByRow);
        Texture2D(int width, int height, nonstd::span<uint8_t const> singleChannelPixelsRowByRow);
        Texture2D(int width, int height, nonstd::span<uint8_t const> channelsRowByRow, int numChannels);
        Texture2D(Texture2D const&);
        Texture2D(Texture2D&&) noexcept;
        Texture2D& operator=(Texture2D const&);
        Texture2D& operator=(Texture2D&&) noexcept;
        ~Texture2D() noexcept;

        int getWidth() const;
        int getHeight() const;
        float getAspectRatio() const;

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

    private:
        friend void osc::DrawTextureAsImGuiImage(Texture2D&, glm::vec2);
        void* updTextureHandleHACK();  // used by ImGui... for now

        friend class GraphicsBackend;
        friend bool operator==(Texture2D const&, Texture2D const&);
        friend bool operator!=(Texture2D const&, Texture2D const&);
        friend bool operator<(Texture2D const&, Texture2D const&);
        friend std::ostream& operator<<(std::ostream&, Texture2D const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Texture2D const&, Texture2D const&);
    bool operator!=(Texture2D const&, Texture2D const&);
    bool operator<(Texture2D const&, Texture2D const&);
    std::ostream& operator<<(std::ostream&, Texture2D const&);

    Texture2D LoadTexture2DFromImageResource(std::string_view, ImageFlags = ImageFlags_None);
}