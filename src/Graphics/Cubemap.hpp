#pragma once

#include "src/Graphics/CubemapFace.hpp"
#include "src/Graphics/TextureFormat.hpp"
#include "src/Utils/CopyOnUpdPtr.hpp"

#include <nonstd/span.hpp>

namespace osc
{
    class Cubemap final {
    public:
        Cubemap(int32_t width, TextureFormat format);
        Cubemap(Cubemap const&);
        Cubemap(Cubemap&&) noexcept;
        Cubemap& operator=(Cubemap const&);
        Cubemap& operator=(Cubemap&&) noexcept;
        ~Cubemap() noexcept;

        int32_t getWidth() const;
        TextureFormat getTextureFormat() const;
        void setPixelData(CubemapFace, nonstd::span<uint8_t const> channelsRowByRow);

    private:
        friend class GraphicsBackend;
        friend bool operator==(Cubemap const&, Cubemap const&) noexcept;
        friend bool operator!=(Cubemap const&, Cubemap const&) noexcept;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    inline bool operator==(Cubemap const& a, Cubemap const& b) noexcept
    {
        return a.m_Impl == b.m_Impl;
    }

    inline bool operator!=(Cubemap const& a, Cubemap const& b) noexcept
    {
        return a.m_Impl != b.m_Impl;
    }
}
