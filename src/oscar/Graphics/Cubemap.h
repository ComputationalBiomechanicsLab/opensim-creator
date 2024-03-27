#pragma once

#include <oscar/Graphics/CubemapFace.h>
#include <oscar/Graphics/TextureFilterMode.h>
#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Graphics/TextureWrapMode.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <span>

namespace osc
{
    class Cubemap final {
    public:
        Cubemap(int32_t width, TextureFormat format);

        int32_t getWidth() const;
        TextureFormat getTextureFormat() const;

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

        // `data` must match the channel layout, bytes per channel, and
        // width*height of the cubemap, or an exception will be thrown
        void setPixelData(CubemapFace, std::span<const uint8_t>);

        friend bool operator==(const Cubemap&, const Cubemap&) = default;
    private:
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };
}
