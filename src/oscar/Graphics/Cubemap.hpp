#pragma once

#include <oscar/Graphics/CubemapFace.hpp>
#include <oscar/Graphics/TextureFormat.hpp>
#include <oscar/Utils/CopyOnUpdPtr.hpp>

#include <cstddef>
#include <span>

namespace osc
{
    class Cubemap final {
    public:
        Cubemap(int32_t width, TextureFormat format);

        int32_t getWidth() const;
        TextureFormat getTextureFormat() const;

        // `data` must match the channel layout, bytes per channel, and
        // width*height of the cubemap, or an exception will be thrown
        void setPixelData(CubemapFace, std::span<uint8_t const>);

        friend bool operator==(Cubemap const&, Cubemap const&) = default;
    private:
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };
}
